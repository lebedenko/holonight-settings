# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Andrii L <lebeden@gmail.com>
"""Isolated, bounded production settings startup and window acceptance."""
import argparse
import os
from pathlib import Path
import re
import signal
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('executable', type=Path)
parser.add_argument('qml_root', type=Path)
parser.add_argument('mode', choices=['default', 'environment', 'command-line', 'configuration'])
parser.add_argument('--acceptance', action='store_true')
parser.add_argument('--forbid-qml-root', type=Path)
args = parser.parse_args()

with tempfile.TemporaryDirectory(prefix='uqc-settings-') as directory:
    root = Path(directory)
    env = os.environ.copy()
    for key in ('QT_QUICK_CONTROLS_STYLE', 'QT_QUICK_CONTROLS_CONF', 'QT_QUICK_CONTROLS_FALLBACK_STYLE',
                'QML_IMPORT_PATH', 'QML2_IMPORT_PATH', 'QT_PLUGIN_PATH', 'DBUS_SESSION_BUS_ADDRESS',
                'DBUS_STARTER_ADDRESS', 'DBUS_STARTER_BUS_TYPE', 'DESKTOP_STARTUP_ID', 'XDG_ACTIVATION_TOKEN'):
        env.pop(key, None)
    for name in ('config', 'cache', 'data', 'runtime'):
        (root / name).mkdir(mode=0o700)
    env.update(QT_QPA_PLATFORM='offscreen', QT_QPA_PLATFORMTHEME='', QT_STYLE_OVERRIDE='',
               QT_QUICK_BACKEND='software', QT_FORCE_STDERR_LOGGING='1', QML_IMPORT_TRACE='1',
               QT_LOGGING_RULES='*.debug=false;qt.qml.import.debug=true;*.warning=true;*.critical=true',
               HOLONIGHT_APPEARANCE_FILE=str(root / 'appearance.toml'),
               XDG_CONFIG_HOME=str(root / 'config'), XDG_CONFIG_DIRS=str(root / 'config'),
               XDG_DATA_HOME=str(root / 'data'), XDG_DATA_DIRS=str(root / 'data'),
               XDG_CACHE_HOME=str(root / 'cache'), XDG_RUNTIME_DIR=str(root / 'runtime'),
               PULSE_SERVER='unix:' + str(root / 'unavailable-audio'))
    command = [str(args.executable.resolve())]
    style = 'Holonight' if args.mode == 'default' else 'Fusion'
    if args.mode == 'environment':
        env['QT_QUICK_CONTROLS_STYLE'] = 'Fusion'
    elif args.mode == 'command-line':
        env['QT_QUICK_CONTROLS_STYLE'] = 'Holonight'
        command += ['-style', 'Fusion']
    elif args.mode == 'configuration':
        config = root / 'qtquickcontrols2.conf'
        config.write_text('[Controls]\nStyle=Fusion\n')
        env['QT_QUICK_CONTROLS_CONF'] = str(config)

    # exec preserves the shell PID for /proc inspection and explicit termination,
    # while dbus-run-session owns and reaps a private daemon.
    pid_file = root / 'app.pid'
    command = ['dbus-run-session', '--', 'sh', '-c',
               'echo $$ > "$1"; shift; exec "$@"', 'settings-check', str(pid_file), *command]
    with (root / 'output.log').open('w+') as log:
        process = subprocess.Popen(command, env=env, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
        try:
            if args.acceptance:
                status = process.wait(timeout=30)
                log.seek(0)
                trace = log.read()
                assert status == 0 and 'ACCEPTANCE_OK' in trace, trace
                mappings = trace.split('MAPS_BEGIN\n')[1].split('MAPS_END')[0]
                assert all('PAGE ' + page in trace for page in
                           ('appearance', 'bar', 'weather', 'integrations', 'audio', 'about')), trace
            else:
                try:
                    status = process.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    pid = int(pid_file.read_text())
                    mappings = Path(f'/proc/{pid}/maps').read_text()
                    os.kill(pid, signal.SIGTERM)
                    assert process.wait(timeout=5) in (-signal.SIGTERM, 128 + signal.SIGTERM), 'Unexpected exit after termination'
                else:
                    log.seek(0)
                    raise AssertionError(f'Premature startup exit {status}:\n{log.read()}')
                log.seek(0)
                trace = log.read()
        finally:
            if process.poll() is None:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()

    if args.forbid_qml_root:
        assert str(args.forbid_qml_root.resolve()) not in trace, 'Installed binary used build QML discovery'

    diagnostics = re.sub(r'MAPS_BEGIN\n.*?MAPS_END\n', '', trace, flags=re.S).splitlines()
    allowed_audio = 'holonight.audio.backend: PulseAudioBackend: pa_context_connect failed: Connection refused'
    errors = [line for line in diagnostics if line and not line.startswith(('qt.qml.import:', 'PAGE ', 'ORIGIN '))
              and line not in (allowed_audio, 'This plugin does not support raise()', 'ACCEPTANCE_OK')]
    assert not errors, 'Unexpected diagnostics:\n' + '\n'.join(errors)
    style_path = '/Holonight/' if style == 'Holonight' else '/QtQuick/Controls/Fusion/'
    for control in ('Button', 'Switch', 'Slider'):
        assert re.search(r'resolveType: .* "Controls\.' + control + r'".*' +
                         re.escape(style_path + control + '.qml'), trace), trace
    modules = ['Core/libholonight_core_qml.so', 'Controls/libholonight_controls_qml.so',
               'impl/libholonight_impl_qml.so']
    if style == 'Holonight':
        modules.append('libholonight_qml.so')
    else:
        assert '/Holonight/libholonight_qml.so' not in mappings, 'Fusion loaded HoloNight style'
    for module in modules:
        suffix = '/Holonight/' + module
        paths = [line.split()[-1] for line in mappings.splitlines() if suffix in line]
        expected = str(args.qml_root.resolve()) + suffix
        assert paths and all(path == expected for path in paths), (expected, paths)
    if not args.acceptance:
        assert not (root / 'appearance.toml').exists(), 'Startup wrote appearance configuration'
        assert not list((root / 'config').rglob('*.toml')), 'Startup wrote product configuration'
    print(f'{args.executable.name}: {args.mode} selects {style}; window, QML and plugin evidence passed')
