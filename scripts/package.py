#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Create a flat, allowlisted release; never include CHITUBOX binaries/resources."""
import hashlib
import importlib.util
from pathlib import Path
import shutil
import tarfile

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location('installer', ROOT/'install.py')
installer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(installer)
payload = ROOT/'build/payload'
payload.mkdir(parents=True, exist_ok=True)
for name in installer.FILES:
    source = ROOT/'build'/name if name.endswith('.so') else ROOT/'src'/name if name.endswith('.qml') else ROOT/name
    shutil.copyfile(source, payload/name)
    (payload/name).chmod(0o755 if name.endswith('.py') else 0o644)
dist = ROOT/'dist'
dist.mkdir(exist_ok=True)
archive = dist/installer.ASSET
with tarfile.open(archive, 'w:gz') as output:
    for name in installer.FILES:
        info = output.gettarinfo(str(payload/name), arcname=name)
        info.uid = info.gid = 0
        info.uname = info.gname = ''
        info.mtime = 0
        with (payload/name).open('rb') as content:
            output.addfile(info, content)
(dist/'SHA256SUMS').write_text(hashlib.sha256(archive.read_bytes()).hexdigest()+'  '+archive.name+'\n')
print(archive)
