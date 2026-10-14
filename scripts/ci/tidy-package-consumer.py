#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Select explicit main and standalone consumer contexts for strict owned-source tidy."""
import json
import os
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
entries = []
for build in ('test', 'package-consumer'):
    entries.extend(json.loads((root / 'build' / build / 'compile_commands.json').read_text()))
database = root / 'build/tidy-commands.json'
database.write_text(json.dumps(entries))
subprocess.run([sys.executable, str(root / 'tooling/workflow.py'), 'tidy', '--scope', 'all'],
               env=dict(os.environ, HOLONIGHT_TIDY_DATABASE=str(database)), check=True)
