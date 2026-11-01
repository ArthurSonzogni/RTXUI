# Package registry recipes

Each directory holds RTXUI's recipe for one registry, laid out at the path it
takes there. `@VERSION@`, `@SHA256@` and `@SHA512@` stand for the release
version and the checksums of GitHub's archive of its tag
(`https://github.com/ArthurSonzogni/RTXUI/archive/refs/tags/v<version>.tar.gz`).

`scripts/publish_registry.py <registry> <tag>` fills them in and hands them
over. `.github/workflows/publish.yml` runs it after each release.

| Registry | Directory | After each release |
| :--- | :--- | :--- |
| [vcpkg](https://github.com/microsoft/vcpkg) | `vcpkg/` | publish.yml opens the bump PR |
| [Bazel Central Registry](https://registry.bazel.build) | `/.bcr/` | publish.yml, through publish-to-bcr |
| [xmake-repo](https://github.com/xmake-io/xmake-repo) | `xmake/` | publish.yml opens the bump PR |
| Homebrew tap ([ArthurSonzogni/homebrew-tap](https://github.com/ArthurSonzogni/homebrew-tap)) | `homebrew/` | publish.yml tests and pushes the formula |
| [AUR](https://aur.archlinux.org/packages/rtxui) | `aur/` | publish.yml pushes, once `AUR_SSH_PRIVATE_KEY` is set |
| [conda-forge](https://github.com/conda-forge/staged-recipes) | `conda-forge/` | regro-cf-autotick-bot opens the bump PR on the feedstock |
| [nixpkgs](https://github.com/NixOS/nixpkgs) | `nixpkgs/` | r-ryantm opens the bump PR |

A package the registry already has is bumped in place: its maintainers may
have edited it since. These templates are only copied for a first
submission, or rewritten whole where RTXUI owns the package (the tap, the AUR).

## Not yet

- **Conan Center** (TODO, adoption): the inclusion criteria ask for adoption
  beyond the author, with no fixed bar; aim for ~200 stars and some outside
  users. Start from `/conanfile.py` and `/packaging/conan/test_package`.
- **homebrew-core** (TODO, adoption): 225 stars, 90 forks or 90 watchers for a
  formula submitted by the project's owner. Then submit `homebrew/Formula/rtxui.rb`.
- **Fedora COPR** (TODO, account): needs a Fedora account and a COPR project.
