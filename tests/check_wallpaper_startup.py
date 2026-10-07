# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 HoloNight contributors
"""Run both window opening orders with isolated configuration and a private bus."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

with tempfile.TemporaryDirectory(prefix='wallpaper-acceptance-') as directory:
    root = Path(directory)
    env = os.environ.copy()
    for name in ('config', 'data', 'runtime', 'cache'):
        (root / name).mkdir(mode=0o700)
    env.update(QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software',
               XDG_CONFIG_HOME=str(root / 'config'), XDG_DATA_HOME=str(root / 'data'),
               XDG_DATA_DIRS=str(root / 'data'), XDG_CONFIG_DIRS=str(root / 'config'),
               XDG_RUNTIME_DIR=str(root / 'runtime'), XDG_CACHE_HOME=str(root / 'cache'),
               HOLONIGHT_APPEARANCE_FILE=str(root / 'appearance.toml'),
               PULSE_SERVER='unix:' + str(root / 'no-audio'))
    result = subprocess.run(['dbus-run-session', '--', sys.argv[1], *sys.argv[2:]],
                            env=env, capture_output=True, text=True, timeout=25)
    print(result.stdout, end='')
    print(result.stderr, end='', file=sys.stderr)
    assert result.returncode == 0 and 'WALLPAPER_ACCEPTANCE_OK' in result.stdout
    allowed = ('This plugin does not support raise()',
               'holonight.audio.backend: PulseAudioBackend: pa_context_connect failed: Connection refused')
    errors = [line for line in result.stderr.splitlines() if line not in allowed]
    assert not errors, '\n'.join(errors)
