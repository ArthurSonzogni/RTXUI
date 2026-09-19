# Consuming RTXUI through vcpkg

This directory holds an **overlay port** for RTXUI. It builds the local checkout
it ships inside, allowing projects to consume RTXUI directly through vcpkg.

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

## Submitting to microsoft/vcpkg
 
Now that RTXUI is public, this port is ready to be submitted to the official
[`microsoft/vcpkg`](https://github.com/microsoft/vcpkg) curated registry:
1. Replace the local `SOURCE_PATH` block in `portfile.cmake` with `vcpkg_from_github`:
   ```cmake
   vcpkg_from_github(
     OUT_SOURCE_PATH SOURCE_PATH
     REPO ArthurSonzogni/RTXUI
     REF "v${VERSION}"
     SHA512 <sha512 of release tarball>
     HEAD_REF main
   )
   ```
2. Copy `packaging/vcpkg/rtxui` to `ports/rtxui` in a `microsoft/vcpkg` clone.
3. Register the version using `vcpkg x-add-version rtxui`.
4. Open a pull request against `microsoft/vcpkg`.

`supports` currently excludes Windows, matching the library: the terminal
backend is POSIX and there is no ConPTY implementation yet.

