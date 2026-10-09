# Copyright (c) 2026 ReLLL and contributors
# SPDX-License-Identifier: GPL-3.0-or-later
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest import mock

spec = importlib.util.spec_from_file_location("installer", Path(__file__).parents[1] / "scripts/install.py")
installer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(installer)


class InstallerTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.binary = self.root / "new.dylib"
        self.binary.write_bytes(b"new plugin")
        self.config = self.root / "config.json"
        self.original = {"modules": ["other"], "moduleInstances": {}, "menuElements": [{"name": "Radio", "open": True}], "frequency": 7000000}
        self.config.write_text(json.dumps(self.original))

    def test_install_update_backup_and_idempotence(self):
        backup = installer.install(self.binary, self.root)
        self.assertEqual(json.loads((backup / "config.json").read_text()), self.original)
        target = self.root / "plugins/cw_notebook.dylib"
        self.assertEqual(target.read_bytes(), b"new plugin")
        self.binary.write_bytes(b"updated plugin")
        backup = installer.install(self.binary, self.root)
        self.assertEqual((backup / target.name).read_bytes(), b"new plugin")
        data = json.loads(self.config.read_text())
        self.assertEqual(data["modules"].count(str(target)), 1)
        self.assertEqual(sum(x["name"] == "CW Notebook" for x in data["menuElements"]), 1)
        self.assertEqual(data["frequency"], 7000000)

    def test_bad_config_changes_nothing(self):
        self.config.write_text('{"modules": "bad"}')
        with self.assertRaises(ValueError):
            installer.install(self.binary, self.root)
        self.assertEqual(self.config.read_text(), '{"modules": "bad"}')
        self.assertFalse((self.root / "plugins").exists())

    def test_config_failure_restores_previous_plugin(self):
        installer.install(self.binary, self.root)
        previous = self.config.read_bytes()
        self.binary.write_bytes(b"update")
        replace = installer.os.replace
        def fail_config(source, dest):
            if Path(dest) == self.config:
                raise OSError("simulated config failure")
            return replace(source, dest)
        with mock.patch.object(installer.os, "replace", side_effect=fail_config):
            with self.assertRaises(OSError):
                installer.install(self.binary, self.root)
        self.assertEqual(self.config.read_bytes(), previous)
        self.assertEqual((self.root / "plugins/cw_notebook.dylib").read_bytes(), b"new plugin")


if __name__ == "__main__":
    unittest.main()
