# Consuming RTXUI through vcpkg

This directory holds an **overlay port**. It is not published anywhere: while
the repository is private it cannot go to the public vcpkg registry, because a
registry port fetches a source archive with no credentials. The port here
builds the checkout it ships inside, so anyone who already has the repository
can consume RTXUI by name today.

## Using it

Point vcpkg at this directory, either on the command line:

```bash
vcpkg install rtxui --overlay-ports=/path/to/RTXUI/packaging/vcpkg
```

or from a manifest project, with a `vcpkg-configuration.json` next to your
`vcpkg.json`:

```json
{
  "overlay-ports": [ "/path/to/RTXUI/packaging/vcpkg" ]
}
```

Then consume it as usual:

```cmake
find_package(rtxui CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE rtxui::rtxui)
```

## Without vcpkg

The same package works from a plain install, which is what the port drives:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix /your/prefix
```

```bash
cmake -B build -DCMAKE_PREFIX_PATH=/your/prefix
```

Debug and Release can share one prefix: debug artifacts carry a `d` suffix
(`librtxuid.a`), so `find_package` selects the right one per configuration.

## Publishing later

When the repository becomes public, this port is what gets submitted to
`microsoft/vcpkg` — the manifest and the build steps are already right. Only
source acquisition changes: swap the local `SOURCE_PATH` block in
`portfile.cmake` for the `vcpkg_from_github` call commented directly beneath
it, filling in the release tag and its SHA512, and add a version entry under
`versions/`.

`supports` currently excludes Windows, matching the library: the terminal
backend is POSIX and there is no ConPTY implementation yet.
