#!/bin/sh
# SPDX-License-Identifier: MIT
set -eu
cd "$(dirname "$0")/.."
: "${CHITUBOX_DIR:?Set CHITUBOX_DIR to your CHITUBOX installation directory}"
qtlib="$CHITUBOX_DIR/lib"
mkdir -p build
c++ -std=c++17 -O2 -fPIC -shared -Wall -Wextra -DQT_NO_VERSION_TAGGING \
 $(pkg-config --cflags Qt6Widgets Qt6Quick Qt6Qml) src/line_support.cpp \
 -L"$qtlib" -Wl,-z,defs -l:libQt6Widgets.so.6 -l:libQt6Quick.so.6 \
 -l:libQt6Qml.so.6 -l:libQt6Gui.so.6 -l:libQt6Core.so.6 -ldl \
 -o build/libchitubox-line-support.so
c++ -std=c++17 -O2 -fPIC -shared -Wall -Wextra -Werror -DQT_NO_VERSION_TAGGING \
 $(pkg-config --cflags Qt6Gui) src/touchpad.cpp \
 -L"$qtlib" -Wl,-z,defs -l:libQt6Gui.so.6 -l:libQt6Core.so.6 -ldl \
 -o build/libchitubox-touchpad.so
