#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 SSI
"""Launch the user's CHITUBOX with its own Qt runtime and our optional extensions."""
import hashlib
import json
import os
from pathlib import Path
import sys


def environment(root, payload, current, touchpad=True, vanilla=False):
    env = dict(current)
    for key in ('QT_PLUGIN_PATH', 'QT_QPA_PLATFORM_PLUGIN_PATH', 'QT_STYLE_OVERRIDE',
                'QT_QPA_PLATFORMTHEME', 'QML_IMPORT_PATH', 'QML2_IMPORT_PATH'):
        env.pop(key, None)
    env.update(QT_QPA_PLATFORM='xcb', QTDIR=str(root), CQT_PKG_ROOT=str(root),
               QML_IMPORT_PATH=str(root/'qml'), QML2_IMPORT_PATH=str(root/'qml'),
               QT_PLUGIN_PATH=str(root/'plugins'),
               QT_QPA_PLATFORM_PLUGIN_PATH=str(root/'plugins/platforms'),
               QTWEBENGINEPROCESS_PATH=str(root/'bin/QtWebEngineProcess'))
    env['PATH'] = str(root/'bin') + os.pathsep + env.get('PATH', '')
    env['LD_LIBRARY_PATH'] = os.pathsep.join([str(payload), str(root/'lib'), str(root), env.get('LD_LIBRARY_PATH', '')])
    # LD_PRELOAD splits on spaces as well as colons; search by basename to support spaces in HOME.
    addons = []
    if not vanilla:
        if env.get('CHITUBOX_LINE_SUPPORT_DISABLE') != '1':
            addons.append('libchitubox-line-support.so')
        if touchpad and env.get('CHITUBOX_TOUCHPAD_DISABLE') != '1':
            addons.append('libchitubox-touchpad.so')
    # Avoid loading legacy copies when invoked from an earlier personal wrapper.
    existing = [p for p in env.get('LD_PRELOAD', '').split(':') if p and
                Path(p).name not in ('libchitubox-line-support.so', 'libchitubox-touchpad.so')]
    if addons or existing:
        env['LD_PRELOAD'] = ':'.join(addons + existing)
    else:
        env.pop('LD_PRELOAD', None)
    return env


def main():
    payload = Path(__file__).resolve().parent
    config = json.loads((payload/'installation.json').read_text())
    root = Path(config['application'])
    vanilla = '--vanilla' in sys.argv[1:]
    args = [a for a in sys.argv[1:] if a != '--vanilla']
    if not vanilla:
        expected = json.loads((payload/'compatibility.json').read_text())['libraries']
        if any(not (root/'lib'/name).is_file() or
               hashlib.sha256((root/'lib'/name).read_bytes()).hexdigest() != digest
               for name, digest in expected.items()):
            print('CHITUBOX changed: extensions disabled. Reinstall a compatible extension version.', file=sys.stderr)
            vanilla = True
    env = environment(root, payload, os.environ, config['touchpad'], vanilla)
    env['CQT_RUN_FILE'] = str(payload/'launch.py')
    executable = root/'bin/CHITUBOX_Basic'
    # Convert local relative filenames before changing the working directory.
    args = [str(Path(a).resolve()) if not a.startswith('-') and Path(a).exists() else a for a in args]
    os.chdir(root/'bin')
    os.execve(executable, [str(executable), *args], env)


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, KeyError) as error:
        print(f'Cannot launch CHITUBOX: {error}', file=sys.stderr)
        sys.exit(1)
