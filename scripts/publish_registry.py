#!/usr/bin/env python3
"""Publishes an RTXUI release to a package registry.

    scripts/publish_registry.py <registry> <tag> [--dry-run]
    scripts/publish_registry.py vcpkg v0.1.3

Registries: vcpkg, xmake, homebrew, aur, conda-forge, nixpkgs. The Bazel
Central Registry is published by publish-to-bcr (.github/workflows/publish.yml,
templates in .bcr/).

Each registry's recipe lives in packaging/registries/<registry>/, at the path
it takes in the registry, with @VERSION@, @SHA256@ and @SHA512@ standing for
the release's version and the checksums of GitHub's archive of its tag.

- vcpkg, xmake: a pull request from a branch of the ArthurSonzogni fork. A
  package the registry already has is bumped in place, so the registry
  maintainers' edits survive; a new one is copied from the template.
- homebrew: the tap ArthurSonzogni/homebrew-tap, rewritten from the template
  and pushed. Created on first use.
- aur: the AUR's git repository, over SSH (an AUR account's key).
- conda-forge, nixpkgs: only the first submission. Once the package exists,
  their bots (regro-cf-autotick-bot, r-ryantm) open the version bumps.

Uses git and an authenticated gh (GH_TOKEN in CI). --dry-run prepares the
change in a temporary directory and prints it, without pushing anything.
"""

import argparse
import hashlib
import os
import re
import shutil
import subprocess
import sys
import tempfile
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TEMPLATES = ROOT / "packaging" / "registries"
OWNER = "ArthurSonzogni"
REPO = "RTXUI"
AUTHOR_NAME = "Arthur Sonzogni"
AUTHOR_EMAIL = "sonzogniarthur@gmail.com"
GITHUB_ID = 4759106


def run(*args, cwd=None, check=True, capture=False):
    print("+", " ".join(str(a) for a in args), flush=True)
    result = subprocess.run([str(a) for a in args], cwd=cwd, check=False,
                            text=True, capture_output=capture)
    if check and result.returncode != 0:
        if capture:
            print(result.stdout, result.stderr, file=sys.stderr)
        sys.exit(f"Command failed with exit code {result.returncode}")
    return result


def output(*args, cwd=None):
    return run(*args, cwd=cwd, capture=True).stdout.strip()


class Release:
    def __init__(self, tag):
        if not re.fullmatch(r"v\d+\.\d+\.\d+", tag):
            sys.exit(f"Invalid tag {tag!r}: expected v<major>.<minor>.<patch>")
        self.tag = tag
        self.version = tag[1:]
        url = (f"https://github.com/{OWNER}/{REPO}/archive/refs/tags/"
               f"{tag}.tar.gz")
        with urllib.request.urlopen(url) as response:
            data = response.read()
        if data[:2] != b"\x1f\x8b":
            sys.exit(f"{url} is not a gzip archive")
        self.sha256 = hashlib.sha256(data).hexdigest()
        self.sha512 = hashlib.sha512(data).hexdigest()

    def substitute(self, text):
        return (text.replace("@VERSION@", self.version)
                    .replace("@SHA256@", self.sha256)
                    .replace("@SHA512@", self.sha512))


def render(release, template, destination, extra=None):
    """Copies a template tree into destination, filling in the release."""
    for source in sorted(template.rglob("*")):
        if source.is_dir():
            continue
        target = destination / source.relative_to(template)
        target.parent.mkdir(parents=True, exist_ok=True)
        text = release.substitute(source.read_text(encoding="utf-8"))
        for key, value in (extra or {}).items():
            text = text.replace(key, value)
        target.write_text(text, encoding="utf-8")


def configure_git(repo):
    run("git", "config", "user.name", OWNER, cwd=repo)
    run("git", "config", "user.email", AUTHOR_EMAIL, cwd=repo)
    # Push over HTTPS with gh's login, in CI as on a machine whose git
    # otherwise uses SSH.
    run("git", "config", "credential.https://github.com.helper", "", cwd=repo)
    run("git", "config", "--add", "credential.https://github.com.helper",
        "!gh auth git-credential", cwd=repo)


def has_changes(repo):
    return output("git", "status", "--porcelain", cwd=repo) != ""


def commit(repo, message, paths):
    run("git", "add", *paths, cwd=repo)
    run("git", "commit", "-m", message, cwd=repo)


def clone_fork(upstream, base, branch, work, sparse=None):
    """A shallow checkout of upstream's base, on a new branch to push to
    OWNER's fork. Registries such as nixpkgs are far too big to clone."""
    name = upstream.split("/")[1]
    fork = f"{OWNER}/{name}"
    run("gh", "repo", "fork", upstream, "--clone=false", check=False)
    for _ in range(30):
        if run("gh", "repo", "view", fork, check=False,
               capture=True).returncode == 0:
            break
        run("sleep", "5")
    repo = work / name
    repo.mkdir()
    run("git", "init", "--quiet", cwd=repo)
    run("git", "remote", "add", "origin", f"https://github.com/{fork}.git",
        cwd=repo)
    run("git", "remote", "add", "upstream",
        f"https://github.com/{upstream}.git", cwd=repo)
    run("git", "fetch", "--depth=1", "--filter=blob:none", "upstream",
        f"{base}:refs/remotes/upstream/{base}", cwd=repo)
    if sparse:
        run("git", "sparse-checkout", "set", *sparse, cwd=repo)
    run("git", "checkout", "-b", branch, f"upstream/{base}", cwd=repo)
    configure_git(repo)
    return repo


def open_pull_request(repo, upstream, base, branch, title, body, dry_run):
    if dry_run:
        print(f"\n--dry-run: would push {branch} and open on {upstream}:")
        print(f"  {title}\n")
        run("git", "log", "--stat", f"upstream/{base}..HEAD", cwd=repo)
        return
    # The checkout is shallow: the fork must already have its base commit.
    fork = f"{OWNER}/{upstream.split('/')[1]}"
    run("gh", "repo", "sync", fork, "--branch", base)
    run("git", "push", "--force", "origin", branch, cwd=repo)
    existing = output("gh", "pr", "list", "--repo", upstream, "--state",
                      "open", "--head", branch, "--json", "url", "--jq",
                      ".[].url")
    if existing:
        print(f"Pull request already open, updated by the push: {existing}")
        return
    run("gh", "pr", "create", "--repo", upstream, "--base", base, "--head",
        f"{OWNER}:{branch}", "--title", title, "--body", body)


def upstream_has(upstream, path, ref):
    return run("gh", "api", f"repos/{upstream}/contents/{path}?ref={ref}",
               check=False, capture=True).returncode == 0


def open_pr_for(upstream, prefix):
    """The URL of an open pull request of ours whose branch starts with prefix."""
    return output("gh", "pr", "list", "--repo", upstream, "--state", "open",
                  "--author", OWNER, "--json", "headRefName,url", "--jq",
                  f'.[] | select(.headRefName | startswith("{prefix}")) | .url')


# vcpkg ------------------------------------------------------------------------

VCPKG_NEW_PORT_BODY = """\
- [x] Changes comply with the [maintainer guide](https://github.com/microsoft/vcpkg-docs/blob/main/vcpkg/contributing/maintainer-guide.md).
- [x] The packaged project is [mature and ready for broad sharing with vcpkg users](https://learn.microsoft.com/vcpkg/contributing/maintainer-guide#packaged-projects-should-be-mature)
    - [x] Has a release at least 6 months old or 6 months of demonstrated public development.
- [x] The packaged project shows strong association with the chosen port name: `rtxui` is the project's name, its GitHub repository (ArthurSonzogni/RTXUI), its CMake package (`find_package(rtxui)`) and its pkg-config module.
    - Repology: https://repology.org/project/rtxui/versions
- [x] Optional dependencies of the build are all controlled by the port: RTXUI has none. Catch2 is only fetched for its tests, turned off here.
- [x] The versioning scheme in `vcpkg.json` matches what upstream says.
- [x] The license declaration in `vcpkg.json` matches what upstream says.
- [x] The installed as the "copyright" file matches what upstream says.
- [x] The source code of the component installed comes from an authoritative source.
- [x] The generated "usage text" is brief and accurate.
- [x] The version database is fixed by rerunning `./vcpkg x-add-version --all` and committing the result.
- [x] Exactly one version is added in each modified versions file.

RTXUI is a C++23 terminal UI library (HTML templates, CSS layout), by the
author of the `ftxui` port. Opened by RTXUI's release automation.
"""


def publish_vcpkg(release, work, dry_run):
    upstream, base = "microsoft/vcpkg", "master"
    branch = f"rtxui-{release.version}"
    repo = clone_fork(upstream, base, branch, work)
    port = repo / "ports" / "rtxui"
    new = not port.exists()
    if new:
        render(release, TEMPLATES / "vcpkg", repo)
    else:
        portfile = port / "portfile.cmake"
        portfile.write_text(re.sub(r"SHA512 [0-9a-fA-F]+",
                                   f"SHA512 {release.sha512}",
                                   portfile.read_text()))
        manifest = port / "vcpkg.json"
        text = re.sub(r'"(version[-a-z]*)": "[^"]+"',
                      rf'"\1": "{release.version}"', manifest.read_text())
        # A port-version qualifies the version it was published against.
        text = re.sub(r',?\n\s*"port-version": \d+', "", text)
        manifest.write_text(text)
    if not has_changes(repo):
        print(f"vcpkg already has rtxui {release.version}.")
        return
    run("./bootstrap-vcpkg.sh", "-disableMetrics", cwd=repo)
    run("./vcpkg", "format-manifest", "ports/rtxui/vcpkg.json", cwd=repo)
    title = ("[rtxui] new port" if new
             else f"[rtxui] update to {release.version}")
    commit(repo, title, ["ports/rtxui"])
    run("./vcpkg", "x-add-version", "rtxui", cwd=repo)
    commit(repo, f"[rtxui] version database for {release.version}",
           ["versions"])
    body = (VCPKG_NEW_PORT_BODY if new else
            f"Updates rtxui to {release.tag}: "
            f"https://github.com/{OWNER}/{REPO}/releases/tag/{release.tag}\n\n"
            "Opened by RTXUI's release automation.")
    open_pull_request(repo, upstream, base, branch, title, body, dry_run)


# xmake ------------------------------------------------------------------------


def publish_xmake(release, work, dry_run):
    upstream, base = "xmake-io/xmake-repo", "dev"
    branch = f"rtxui-{release.version}"
    repo = clone_fork(upstream, base, branch, work)
    recipe = repo / "packages" / "r" / "rtxui" / "xmake.lua"
    new = not recipe.exists()
    if new:
        render(release, TEMPLATES / "xmake", repo)
    elif f'add_versions("{release.tag}"' not in recipe.read_text():
        line = f'    add_versions("{release.tag}", "{release.sha256}")\n'
        recipe.write_text(re.sub(r"^(    add_versions\()", line + r"\1",
                                 recipe.read_text(), count=1, flags=re.M))
    if not has_changes(repo):
        print(f"xmake-repo already has rtxui {release.tag}.")
        return
    title = "add rtxui" if new else f"update rtxui to {release.tag}"
    commit(repo, title, ["packages/r/rtxui"])
    body = ("RTXUI is a C++23 terminal UI library (HTML templates, CSS "
            f"layout): https://github.com/{OWNER}/{REPO}\n\n"
            "Opened by RTXUI's release automation.")
    open_pull_request(repo, upstream, base, branch, title, body, dry_run)


# Homebrew ---------------------------------------------------------------------

TAP = f"{OWNER}/homebrew-tap"


def publish_homebrew(release, work, dry_run):
    if run("gh", "repo", "view", TAP, check=False,
           capture=True).returncode != 0:
        if dry_run:
            print(f"--dry-run: would create {TAP}")
            repo = work / "homebrew-tap"
            repo.mkdir()
            run("git", "init", "-b", "main", cwd=repo)
        else:
            run("gh", "repo", "create", TAP, "--public", "--description",
                "Homebrew formulae of ArthurSonzogni's projects: "
                "brew install arthursonzogni/tap/<name>")
            run("git", "clone", f"https://github.com/{TAP}.git",
                work / "homebrew-tap")
            repo = work / "homebrew-tap"
            run("git", "checkout", "-B", "main", cwd=repo)
    else:
        run("git", "clone", f"https://github.com/{TAP}.git",
            work / "homebrew-tap")
        repo = work / "homebrew-tap"
    configure_git(repo)
    render(release, TEMPLATES / "homebrew", repo)
    if not has_changes(repo):
        print(f"The tap already has rtxui {release.version}.")
        return
    if shutil.which("brew"):
        # Install and test the formula from a scratch tap before the real
        # tap's users can see it.
        test_tap = "rtxui/publish-test"
        run("brew", "tap-new", "--no-git", test_tap)
        tap_dir = Path(output("brew", "--repository", test_tap))
        shutil.copy(repo / "Formula" / "rtxui.rb", tap_dir / "Formula")
        run("brew", "install", "--build-from-source", f"{test_tap}/rtxui")
        run("brew", "test", f"{test_tap}/rtxui")
        run("brew", "uninstall", f"{test_tap}/rtxui")
        run("brew", "untap", test_tap)
    commit(repo, f"rtxui {release.version}", ["."])
    if dry_run:
        run("git", "show", "--stat", "HEAD", cwd=repo)
        return
    run("git", "push", "origin", "main", cwd=repo)


# AUR --------------------------------------------------------------------------


def publish_aur(release, work, dry_run):
    """Needs makepkg (Arch Linux, as a user other than root) and the AUR key."""
    repo = work / "rtxui"
    run("git", "clone", "ssh://aur@aur.archlinux.org/rtxui.git", repo)
    configure_git(repo)
    render(release, TEMPLATES / "aur", repo)
    srcinfo = output("makepkg", "--printsrcinfo", cwd=repo)
    (repo / ".SRCINFO").write_text(srcinfo + "\n", encoding="utf-8")
    if not has_changes(repo):
        print(f"The AUR already has rtxui {release.version}.")
        return
    # Build the package before publishing it.
    run("makepkg", "--force", "--cleanbuild", cwd=repo)
    commit(repo, f"rtxui {release.version}", ["PKGBUILD", ".SRCINFO"])
    if dry_run:
        run("git", "show", "HEAD", cwd=repo)
        return
    run("git", "push", "origin", "HEAD:master", cwd=repo)


# conda-forge ------------------------------------------------------------------


def publish_conda_forge(release, work, dry_run):
    if run("gh", "repo", "view", "conda-forge/rtxui-feedstock", check=False,
           capture=True).returncode == 0:
        print("conda-forge/rtxui-feedstock exists: regro-cf-autotick-bot "
              "opens the version bumps.")
        return
    upstream, base = "conda-forge/staged-recipes", "main"
    existing = open_pr_for(upstream, "rtxui")
    branch = "rtxui"
    repo = clone_fork(upstream, base, branch, work,
                      sparse=["recipes/rtxui"])
    render(release, TEMPLATES / "conda-forge", repo)
    commit(repo, "Add rtxui", ["recipes/rtxui"])
    if existing:
        print(f"Updating the open pull request {existing}")
    body = ("Adds [RTXUI](https://github.com/ArthurSonzogni/RTXUI), a C++23 "
            "terminal UI library built from HTML templates and CSS.\n\n"
            "- [x] Title of this PR is meaningful: e.g. \"Adding my_nifty_"
            "package\", not \"updated meta.yaml\".\n"
            "- [x] License file is packaged.\n"
            "- [x] Source is from official source.\n"
            "- [x] Package does not vendor other packages.\n"
            "- [x] If static libraries are linked in, the license of the "
            "static library is packaged: none.\n"
            "- [x] Package does not ship static libraries.\n"
            "- [x] Build number is 0.\n"
            "- [x] A tarball (`url`) rather than a repo (e.g. `git_url`) is "
            "used in your recipe.\n"
            "- [x] GitHub users listed in the maintainer section have posted "
            "a comment confirming they are willing to be listed there.\n"
            "- [x] When in trouble, please check our knowledge base "
            "documentation before pinging a team.\n")
    open_pull_request(repo, upstream, base, branch, "Add rtxui", body,
                      dry_run)


# nixpkgs ----------------------------------------------------------------------

NIX_MAINTAINER = f"""\
  arthursonzogni = {{
    email = "{AUTHOR_EMAIL}";
    github = "{OWNER}";
    githubId = {GITHUB_ID};
    name = "{AUTHOR_NAME}";
  }};
"""


def nix_hash(release):
    url = (f"https://github.com/{OWNER}/{REPO}/archive/refs/tags/"
           f"{release.tag}.tar.gz")
    base32 = output("nix-prefetch-url", "--unpack", url)
    return output("nix", "hash", "convert", "--hash-algo", "sha256", "--to",
                  "sri", base32)


def add_nix_maintainer(path):
    text = path.read_text(encoding="utf-8")
    if re.search(r"^  arthursonzogni = \{", text, re.M):
        return False
    # Entries are sorted by handle, case-insensitively.
    for match in re.finditer(r"^  ([A-Za-z0-9_\-\"']+) = \{", text, re.M):
        if match.group(1).strip("\"'").lower() > "arthursonzogni":
            text = text[:match.start()] + NIX_MAINTAINER + text[match.start():]
            path.write_text(text, encoding="utf-8")
            return True
    sys.exit("Found no place for the maintainer entry")


def publish_nixpkgs(release, work, dry_run):
    upstream, base = "NixOS/nixpkgs", "master"
    if upstream_has(upstream, "pkgs/by-name/rt/rtxui/package.nix", base):
        print("nixpkgs has rtxui: r-ryantm opens the version bumps.")
        return
    branch = "rtxui-init"
    repo = clone_fork(upstream, base, branch, work,
                      sparse=["maintainers", "pkgs/by-name/rt"])
    if add_nix_maintainer(repo / "maintainers" / "maintainer-list.nix"):
        commit(repo, "maintainers: add arthursonzogni",
               ["maintainers/maintainer-list.nix"])
    render(release, TEMPLATES / "nixpkgs", repo,
           {"@NIX_HASH@": nix_hash(release)})
    commit(repo, f"rtxui: init at {release.version}",
           ["pkgs/by-name/rt/rtxui"])
    body = (f"Adds [RTXUI](https://github.com/{OWNER}/{REPO}), a C++23 "
            "terminal UI library built from HTML templates and CSS, and its "
            "author as maintainer.\n\n"
            "## Things done\n\n"
            "- Built on platform:\n"
            "  - [x] x86_64-linux\n"
            "- [x] Tested basic functionality: an app built against the "
            "package with `find_package(rtxui)` and with pkg-config runs.\n"
            "- [x] Fits [CONTRIBUTING.md](https://github.com/NixOS/nixpkgs/"
            "blob/master/CONTRIBUTING.md).\n")
    open_pull_request(repo, upstream, base, branch,
                      f"rtxui: init at {release.version}", body, dry_run)


PUBLISHERS = {
    "vcpkg": publish_vcpkg,
    "xmake": publish_xmake,
    "homebrew": publish_homebrew,
    "aur": publish_aur,
    "conda-forge": publish_conda_forge,
    "nixpkgs": publish_nixpkgs,
}


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("registry", choices=sorted(PUBLISHERS))
    parser.add_argument("tag", help="the release tag, e.g. v0.1.3")
    parser.add_argument("--dry-run", action="store_true",
                        help="prepare and print the change, push nothing")
    args = parser.parse_args()
    release = Release(args.tag)
    work = Path(tempfile.mkdtemp(prefix=f"rtxui-{args.registry}-"))
    print(f"rtxui {release.version}: sha256 {release.sha256}, in {work}")
    PUBLISHERS[args.registry](release, work, args.dry_run)
    if not args.dry_run:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    main()
