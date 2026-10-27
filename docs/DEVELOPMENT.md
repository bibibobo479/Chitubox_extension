# Build and validate

The source is MIT; the separately installed host is proprietary and is not downloaded by the build or CI.

## Build

Dependencies: C++17 compiler, pkg-config, Python 3.9+, Qt 6 development headers for Core/Gui/Widgets/Qml/Quick, and the supported native CHITUBOX build. On Arch, development headers are provided by `qt6-base` and `qt6-declarative`. Other distributions package headers separately. Use a compatible Qt ABI; successful compilation alone does not establish runtime compatibility.

```bash
export CHITUBOX_DIR="/path/to/chitubox-basic-2.3.1"
./scripts/build.sh
./scripts/test.sh
python3 scripts/package.py
python3 install.py --payload build/payload --app "$CHITUBOX_DIR"
```

The current installer requires glibc 2.38+ even for local builds. Published binaries were built with GCC / Qt 6 development headers and dynamically linked to the host's bundled Qt runtime, using `QT_NO_VERSION_TAGGING`. No Qt library is statically linked or included. The `.so` files have no personal absolute RPATH. You can rebuild them against compatible modified Qt libraries; keep the host ABI compatible.

`build/payload` is the flat installation payload. `dist/` contains the release archive and SHA256SUMS. Packaging uses an explicit allowlist: no vendor binaries, extracted resources, private profiles, research dumps or local settings. Source is published through the matching Git tag; GitHub also provides source archives.

## Tests

`./scripts/test.sh` always runs surface-planning tests and Python installer/launcher tests. If `CHITUBOX_DIR` is set, it additionally builds and runs the touchpad input-routing test against bundled Qt. No CHITUBOX window is opened by these tests.

The geometry suite covers spacing, last endpoint, holes, surface normals, a 40 mm protrusion, a 40 mm recess, and bounds. Python tests cover installation/update/removal, preserved user data, paths with spaces/non-ASCII characters, unsupported host hashes, unrelated launchers, symlinks, archive allowlisting, checksum rejection and launcher environment isolation. The touchpad test verifies forwarding of ordinary input and rotation only for the intended gesture.

### Optional native GUI regression

`tests/gui.cpp` drives a separate CHITUBOX instance with the authored `tests/boss.stl` fixture. It needs an initialized printer/guide configuration in a **dedicated** XDG profile. Do not point it at a real project. It assumes the initial default view/window geometry used in the validation environment; coordinates may need adjustment on another display.

Build the test probe:

```bash
c++ -std=c++17 -O2 -fPIC -shared -DQT_NO_VERSION_TAGGING \
  $(pkg-config --cflags Qt6Widgets Qt6Quick) tests/gui.cpp \
  -L"$CHITUBOX_DIR/lib" -Wl,-z,defs -l:libQt6Widgets.so.6 \
  -l:libQt6Quick.so.6 -l:libQt6Gui.so.6 -l:libQt6Core.so.6 -ldl \
  -o build/gui-test.so
```

After installing the payload, launch its wrapper with separate `XDG_CONFIG_HOME`, `XDG_DATA_HOME`, `XDG_CACHE_HOME`, `LD_LIBRARY_PATH="$CHITUBOX_DIR/lib"`, and `LD_PRELOAD="$PWD/build/gui-test.so"`, passing the absolute fixture path. Use the absolute installed `launch.py` path so that changing XDG variables doesn't select another installation. It checks two consecutive rows, a collapsed active panel, Escape behavior, single-row undo/redo, native Add selection and keyboard mode changes. It exits its own process after testing.

## Architecture

- `src/geometry.hpp`: host-independent surface projection and planning.
- `src/native.hpp`: version-specific internal ABI bridge and grouped undo.
- `src/line_support.cpp`: Qt event filter, persistent tool, native hover capture, sidebar injection and host hashes.
- `src/sidebar.qml`: original sidebar UI, inserted after the host's Auto Supports button.
- `src/touchpad.cpp`: Ctrl-touchpad event routing with pass-through for other events.
- `install.py`, `launch.py`: per-user install and runtime setup; no third-party Python dependencies.

Support preview calls supply the two model-space endpoints and normals. Each planned contact is projected onto the outermost facing triangle, then passed to the native support generator. Successfully created components are committed as one undo operation. The native algorithm still decides whether a particular support is valid.

When adding support for another CHITUBOX version, audit all native ABI assumptions and test in a dedicated profile before adding hashes. Do not simply remove hash checks. Never commit proprietary resources or a GitHub token.

## Release

Run build/tests/native validation, update validation notes, package, then commit and tag the matching source. Upload `dist/chitubox-extension-linux-x86_64.tar.gz` and `dist/SHA256SUMS` to the same GitHub release. Keep LICENSE, NOTICE and LGPL/GPL texts in every payload. The CI workflow runs only tests independent of the proprietary host.
