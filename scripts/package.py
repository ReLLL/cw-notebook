#!/usr/bin/env python3
# Copyright (c) 2026 ReLLL and contributors
# SPDX-License-Identifier: GPL-3.0-or-later
"""Package an explicit public release allowlist; never scan private folders."""
import argparse
import hashlib
from pathlib import Path
import re
import zipfile

ROOT = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--binary", type=Path, required=True)
args = parser.parse_args()
version = re.search(r"project\(cw_notebook VERSION ([0-9.]+)", (ROOT / "CMakeLists.txt").read_text()).group(1)
if not args.binary.is_file():
    parser.error("Binary not found; build Release first.")
name = f"cw-notebook-{version}-macos-arm64"
folder = ROOT / "dist"
folder.mkdir(exist_ok=True)
archive = folder / f"{name}.zip"
files = {"cw_notebook.dylib": args.binary}
for path in ("scripts/install.py", "README.md", "LICENSE", "CREDITS.md", "THIRD_PARTY_NOTICES.md", "CHANGELOG.md"):
    files[path] = ROOT / path
with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as output:
    for relative, source in files.items():
        output.write(source, f"{name}/{relative}")
digest = hashlib.sha256(archive.read_bytes()).hexdigest()
(folder / "SHA256SUMS.txt").write_text(f"{digest}  {archive.name}\n")
print(archive)
print(folder / "SHA256SUMS.txt")
