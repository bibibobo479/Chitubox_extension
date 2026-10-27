#!/bin/sh
# SPDX-License-Identifier: MIT
set -eu
cd "$(dirname "$0")/.."
mkdir -p build
c++ -std=c++17 -O2 -Wall -Wextra -Werror tests/geometry_test.cpp -o build/test-geometry
build/test-geometry
python3 -m unittest discover -s tests -p 'test_*.py' -v
if [ -n "${CHITUBOX_DIR:-}" ]; then
 qtlib="$CHITUBOX_DIR/lib"
 c++ -std=c++17 -O2 -fPIC -shared -DQT_NO_VERSION_TAGGING $(pkg-config --cflags Qt6Gui) tests/backend.cpp -L"$qtlib" -l:libQt6Gui.so.6 -l:libQt6Core.so.6 -o build/libtest-backend.so
 c++ -std=c++17 -O2 -DQT_NO_VERSION_TAGGING $(pkg-config --cflags Qt6Gui) tests/routing.cpp -Lbuild -ltest-backend -L"$qtlib" -l:libQt6Gui.so.6 -l:libQt6Core.so.6 -o build/test-routing
 LD_LIBRARY_PATH="$qtlib:$PWD/build" QT_QPA_PLATFORM=offscreen QT_PLUGIN_PATH="$CHITUBOX_DIR/plugins" LD_PRELOAD="$PWD/build/libchitubox-touchpad.so" build/test-routing
fi
