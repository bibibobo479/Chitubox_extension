#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 SSI
"""Install prebuilt extensions per user. No root, vendor downloads, or pip dependencies."""
import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import platform
import shlex
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.request

REPO = 'bibibobo479/Chitubox_extension'
ASSET = 'chitubox-extension-linux-x86_64.tar.gz'
MARKER = '# Managed by Chitubox_extension'
FILES = ('libchitubox-line-support.so', 'libchitubox-touchpad.so', 'sidebar.qml',
         'launch.py', 'install.py', 'compatibility.json', 'LICENSE', 'NOTICE.md', 'README.md',
         'LGPL-3.0.txt', 'GPL-3.0.txt')
EXPECTED = {
    'libCbdKernel.so.1': '5dbcedd7f0e90b91f7e0e34c4b98b48506fb891fda6b131921e2beaf34c241b1',
    'libCbdModel.so.1': '05185ca2e4c76d6142ee3450e6efd43dae7ea22e0fc3a9150fc27b99180bf711',
}


def data_home():
    return Path(os.environ.get('XDG_DATA_HOME', str(Path.home()/'.local/share')))


def locations():
    data = data_home()
    return data/'chitubox-extension', Path.home()/'.local/bin/chitubox-extended', data/'applications/chitubox-extension.desktop'


def atomic(path, content, mode=0o644):
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, name = tempfile.mkstemp(prefix='.chitu-', dir=path.parent)
    try:
        with os.fdopen(fd, 'wb') as output:
            output.write(content)
        os.chmod(name, mode)
        os.replace(name, path)
    finally:
        if os.path.exists(name):
            os.unlink(name)


def app_root(path):
    path = Path(path).expanduser().resolve()
    if path.is_file():
        path = path.parent.parent if path.parent.name == 'bin' else path.parent
    return path if (path/'bin/CHITUBOX_Basic').is_file() and (path/'lib').is_dir() else None


def discover(explicit=None):
    if explicit:
        root = app_root(explicit)
        if root:
            return root
        raise ValueError('--app must name a CHITUBOX folder containing bin/CHITUBOX_Basic and lib/')
    candidates = []
    target, _, _ = locations()
    config = target/'installation.json'
    if config.is_file():
        candidates.append(json.loads(config.read_text()).get('application', ''))
    if os.environ.get('CHITUBOX_DIR'):
        candidates.append(os.environ['CHITUBOX_DIR'])
    for name in ('CHITUBOX_Basic', 'CHITUBOX'):
        if shutil.which(name):
            candidates.append(shutil.which(name))
    for directory in (Path.home()/'.local/opt', Path.home()/'Applications', Path('/opt'), Path('/usr/local')):
        if directory.is_dir():
            for path in sorted(directory.iterdir()):
                if 'chitu' in path.name.lower():
                    candidates.append(path)
                    if path.is_dir():
                        candidates.extend(p for p in path.iterdir() if p.is_dir() and 'chitu' in p.name.lower())
    roots = [r for p in candidates if p and (r := app_root(p))]
    for root in roots:
        if compatible(root):
            return root
    if roots:
        raise ValueError('CHITUBOX found, but this build is unsupported. Required: native Basic 2.3.1 Linux x86_64 (matching hashes).')
    raise ValueError('CHITUBOX not found. Install Basic 2.3.1 first, or rerun with --app "/path/to/CHITUBOX".')


def compatible(root):
    return all((root/'lib'/name).is_file() and
               hashlib.sha256((root/'lib'/name).read_bytes()).hexdigest() == digest
               for name, digest in EXPECTED.items())


def system_check():
    if platform.system() != 'Linux' or platform.machine() not in ('x86_64', 'amd64'):
        raise ValueError('This release requires Linux x86_64; Windows, macOS and ARM are not supported.')
    name, version = platform.libc_ver()
    if name != 'glibc' or tuple(map(int, version.split('.')[:2])) < (2, 38):
        raise ValueError('The prebuilt release requires glibc 2.38+ (e.g. Ubuntu 24.04+, Debian 13, modern Arch/Fedora). See source-build instructions.')


def fetch(url):
    if not url.startswith('https://'):
        raise ValueError('Only HTTPS downloads are allowed')
    request = urllib.request.Request(url, headers={'User-Agent': 'Chitubox-extension-installer/1.0'})
    with urllib.request.urlopen(request, timeout=60) as response:
        result = response.read(32*1024*1024 + 1)
    if len(result) > 32*1024*1024:
        raise ValueError('Release asset exceeds 32 MiB')
    return result


def download(destination):
    release = json.loads(fetch(f'https://api.github.com/repos/{REPO}/releases/latest'))
    assets = {a['name']: a for a in release['assets']}
    if ASSET not in assets or 'SHA256SUMS' not in assets:
        raise ValueError('Latest release is incomplete: binary archive or SHA256SUMS missing')
    checksums = fetch(assets['SHA256SUMS']['browser_download_url']).decode()
    digest = next((line.split()[0] for line in checksums.splitlines()
                   if len(line.split()) == 2 and line.split()[1].lstrip('*') == ASSET), None)
    archive = fetch(assets[ASSET]['browser_download_url'])
    if not digest or hashlib.sha256(archive).hexdigest() != digest:
        raise ValueError('Release checksum mismatch; installation stopped')
    extract(archive, destination)


def extract(archive, destination):
    # Never extract paths, links, devices or arbitrary metadata from an archive.
    with tarfile.open(fileobj=io.BytesIO(archive), mode='r:gz') as source:
        seen = set()
        for member in source.getmembers():
            if member.name not in FILES or not member.isfile() or member.name in seen or member.size > 8*1024*1024:
                raise ValueError(f'Unexpected archive member: {member.name}')
            seen.add(member.name)
            atomic(destination/member.name, source.extractfile(member).read())
        if seen != set(FILES):
            raise ValueError('Incomplete release archive')


def desktop_quote(value):
    # Desktop Entry Exec quoting is distinct from shell quoting.
    value = str(value).replace('%', '%%')
    for char in ('\\', '"', '`', '$'):
        value = value.replace(char, '\\'+char)
    return '"'+value+'"'


def install(payload, root, touchpad=True):
    if not compatible(root):
        raise ValueError('Unsupported CHITUBOX library hashes; nothing was installed')
    for name in FILES:
        if not (payload/name).is_file():
            raise ValueError(f'Missing payload file: {name}')
    target, launcher, desktop = locations()
    if target.is_symlink():
        raise ValueError('Installation directory must not be a symlink')
    for path in (launcher, desktop):
        if path.exists() and MARKER not in path.read_text():
            raise ValueError(f'Refusing to overwrite an unrelated file: {path}')
    for name in FILES:
        atomic(target/name, (payload/name).read_bytes(), 0o755 if name.endswith('.py') else 0o644)
    atomic(target/'installation.json', json.dumps({'application': str(root), 'touchpad': touchpad}, indent=2).encode())
    atomic(launcher, ('#!/bin/sh\n'+MARKER+'\nexec python3 '+shlex.quote(str(target/'launch.py'))+' "$@"\n').encode(), 0o755)
    entry = ('[Desktop Entry]\n'+MARKER+'\nType=Application\nName=CHITUBOX + Extensions\n'
             'Name[ru]=CHITUBOX + расширения\nComment=Line supports and Ctrl+touchpad orbit\n'
             'Exec='+desktop_quote(launcher)+' %F\nIcon=applications-graphics\nTerminal=false\nCategories=Graphics;3DGraphics;\n')
    atomic(desktop, entry.encode())
    if shutil.which('update-desktop-database'):
        subprocess.run(['update-desktop-database', str(desktop.parent)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    print('Installed. Open “CHITUBOX + Extensions” from your application menu. / Готово: ярлык «CHITUBOX + расширения».')
    print(f'Terminal: {launcher}\nUninstall: python3 {shlex.quote(str(target/"install.py"))} --uninstall')
    return target


def uninstall():
    target, launcher, desktop = locations()
    if target.is_symlink():
        raise ValueError('Installation directory must not be a symlink')
    for path in (launcher, desktop):
        if path.is_file() and MARKER in path.read_text():
            path.unlink()
    for name in (*FILES, 'installation.json'):
        path = target/name
        if path.is_file() or path.is_symlink():
            path.unlink()
    try:
        target.rmdir()
    except OSError:
        pass
    print('Extensions removed. CHITUBOX and saved projects were not changed. Settings kept in your XDG config directory.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', help='Existing CHITUBOX installation folder')
    parser.add_argument('--payload', type=Path, help='Use a local extracted release (offline)')
    parser.add_argument('--no-touchpad', action='store_true')
    parser.add_argument('--uninstall', action='store_true')
    args = parser.parse_args()
    if os.geteuid() == 0:
        raise ValueError('Run as your regular desktop user, without sudo')
    if args.uninstall:
        uninstall()
        return
    system_check()
    root = discover(args.app)
    if not compatible(root):
        raise ValueError('Unsupported CHITUBOX build')
    here = Path(__file__).resolve().parent
    payload = args.payload or here
    if (payload/'libchitubox-line-support.so').is_file():
        install(payload, root, not args.no_touchpad)
    elif args.payload:
        raise ValueError('Local payload has no compiled extension')
    else:
        with tempfile.TemporaryDirectory(prefix='chitubox-extension-') as tmp:
            payload = Path(tmp)
            download(payload)
            install(payload, root, not args.no_touchpad)


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, KeyError) as error:
        print(f'Installation stopped: {error}', file=sys.stderr)
        sys.exit(1)
