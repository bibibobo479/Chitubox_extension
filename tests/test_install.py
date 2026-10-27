# SPDX-License-Identifier: MIT
import hashlib
import io
import json
import os
from pathlib import Path
import tarfile
import tempfile
import unittest
from unittest.mock import patch
import install
import launch


class Installation(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.base = Path(self.tmp.name)/'home с пробелами'
        self.base.mkdir()
        self.env = patch.dict(os.environ, {'HOME': str(self.base), 'XDG_DATA_HOME': str(self.base/'data')})
        self.env.start()
        self.app = self.base/'CHITUBOX 2.3.1'
        (self.app/'bin').mkdir(parents=True)
        (self.app/'bin/CHITUBOX_Basic').write_text('test fixture, not a vendor executable')
        (self.app/'lib').mkdir()
        self.expected = {}
        for name in install.EXPECTED:
            content = name.encode()
            (self.app/'lib'/name).write_bytes(content)
            self.expected[name] = hashlib.sha256(content).hexdigest()
        self.hashes = patch.object(install, 'EXPECTED', self.expected)
        self.hashes.start()
        self.payload = self.base/'payload'
        self.payload.mkdir()
        for name in install.FILES:
            (self.payload/name).write_text('original '+name)

    def tearDown(self):
        self.hashes.stop()
        self.env.stop()
        self.tmp.cleanup()

    def test_install_upgrade_uninstall_preserves_user_data(self):
        target = install.install(self.payload, self.app)
        _, launcher, desktop = install.locations()
        self.assertTrue(os.access(launcher, os.X_OK))
        self.assertIn('"$@"', launcher.read_text())
        self.assertIn(str(target/'launch.py'), launcher.read_text())
        self.assertIn('CHITUBOX + Extensions', desktop.read_text())
        self.assertEqual(json.loads((target/'installation.json').read_text())['application'], str(self.app))
        (target/'user-notes.txt').write_text('keep')
        (self.payload/'sidebar.qml').write_text('updated')
        install.install(self.payload, self.app, False)
        self.assertEqual((target/'sidebar.qml').read_text(), 'updated')
        self.assertFalse(json.loads((target/'installation.json').read_text())['touchpad'])
        install.uninstall()
        self.assertFalse(launcher.exists())
        self.assertFalse(desktop.exists())
        self.assertEqual((target/'user-notes.txt').read_text(), 'keep')
        self.assertTrue((self.app/'bin/CHITUBOX_Basic').exists())

    def test_refuses_unknown_libraries_before_install(self):
        (self.app/'lib/libCbdKernel.so.1').write_bytes(b'changed')
        with self.assertRaisesRegex(ValueError, 'Unsupported'):
            install.install(self.payload, self.app)
        self.assertFalse(install.locations()[0].exists())

    def test_preserves_unrelated_launcher(self):
        launcher = install.locations()[1]
        launcher.parent.mkdir(parents=True)
        launcher.write_text('my own launcher')
        with self.assertRaisesRegex(ValueError, 'unrelated'):
            install.install(self.payload, self.app)
        self.assertEqual(launcher.read_text(), 'my own launcher')
        self.assertFalse(install.locations()[0].exists())

    def test_refuses_symlink_installation(self):
        target = install.locations()[0]
        target.parent.mkdir(parents=True)
        target.symlink_to(self.payload, target_is_directory=True)
        with self.assertRaisesRegex(ValueError, 'symlink'):
            install.install(self.payload, self.app)

    def test_discovers_existing_install_and_explicit_binary(self):
        self.assertEqual(install.discover(self.app/'bin/CHITUBOX_Basic'), self.app)
        install.install(self.payload, self.app)
        self.assertEqual(install.discover(), self.app)

    def archive(self, names, link=False):
        buffer = io.BytesIO()
        with tarfile.open(fileobj=buffer, mode='w:gz') as output:
            for name in names:
                info = tarfile.TarInfo(name)
                if link:
                    info.type = tarfile.SYMTYPE
                    info.linkname = '/etc/passwd'
                    output.addfile(info)
                else:
                    info.size = 4
                    output.addfile(info, io.BytesIO(b'test'))
        return buffer.getvalue()

    def test_archive_allowlist_and_complete_payload(self):
        dest = self.base/'extract'
        install.extract(self.archive(install.FILES), dest)
        self.assertEqual({p.name for p in dest.iterdir()}, set(install.FILES))
        for names in (['../escape'], ['/tmp/escape'], ['vendor.so'], ['LICENSE', 'LICENSE'], ['LICENSE']):
            with self.subTest(names=names), self.assertRaises(ValueError):
                install.extract(self.archive(names), dest)
        with self.assertRaises(ValueError):
            install.extract(self.archive(['LICENSE'], link=True), dest)
        self.assertFalse((self.base/'escape').exists())

    def test_download_checks_digest_before_extracting(self):
        release = {'assets': [{'name': name, 'browser_download_url': 'https://example.org/'+name}
                              for name in (install.ASSET, 'SHA256SUMS')]}
        responses = [json.dumps(release).encode(), ('0'*64+'  '+install.ASSET).encode(), b'bad archive']
        with patch.object(install, 'fetch', side_effect=responses), self.assertRaisesRegex(ValueError, 'checksum'):
            install.download(self.base/'not-created')
        self.assertFalse((self.base/'not-created').exists())

    def test_launcher_isolated_qt_and_opt_out(self):
        current = {'LD_PRELOAD': '/old/libchitubox-touchpad.so:/independent.so',
                   'QT_PLUGIN_PATH': '/system/qt', 'QML2_IMPORT_PATH': '/system/qml', 'PATH': '/usr/bin'}
        env = launch.environment(self.app, self.payload, current)
        self.assertEqual(env['LD_PRELOAD'], 'libchitubox-line-support.so:libchitubox-touchpad.so:/independent.so')
        self.assertTrue(env['LD_LIBRARY_PATH'].startswith(str(self.payload)+':'))
        self.assertEqual(env['QT_PLUGIN_PATH'], str(self.app/'plugins'))
        self.assertEqual(env['QT_QPA_PLATFORM'], 'xcb')
        self.assertEqual(current['QT_PLUGIN_PATH'], '/system/qt')
        self.assertEqual(launch.environment(self.app, self.payload, current, vanilla=True)['LD_PRELOAD'], '/independent.so')
        self.assertEqual(launch.environment(self.app, self.payload, {}, touchpad=False)['LD_PRELOAD'], 'libchitubox-line-support.so')
        self.assertNotIn('LD_PRELOAD', launch.environment(self.app, self.payload, {}, vanilla=True))

    def test_desktop_exec_escapes_special_characters(self):
        self.assertEqual(install.desktop_quote('/tmp/a%f"$`'), '"/tmp/a%%f\\"\\$\\`"')


if __name__ == '__main__':
    unittest.main()
