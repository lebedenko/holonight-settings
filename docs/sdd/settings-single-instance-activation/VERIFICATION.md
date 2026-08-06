# Settings Single-Instance Activation — Verification

**Status:** Implemented and verified

## Automated results

- Focused build: `cmake --build build --target test_holonight_settings holonight-settings -j2` — passed.
- Focused activation tests under a private session bus — 6/6 passed. Coverage includes platform-data mapping,
  readiness queueing, disconnected-bus failure, metadata identity, ownership/forwarding, and introspection.
- Full regression: `task test` — 1098/1098 passed (one unrelated environment-dependent test skipped).
- Changed-file clang-format check — passed.
- Repository-wide `task format-check` — blocked by two pre-existing formatting violations in
  `tests/test_settings_app.cpp` (lines reported around 1352 and 1354); no changed file was reported after formatting.

## Live Wayland checklist

- [x] Launch twice through the installed settings launcher/key binding; confirm one process and existing-window
  activation.
- [x] Launch twice directly; confirm one process and existing-window activation.
- [x] Restore a hidden/minimized window through the tested activation path.
- [x] Repeat launch with the window on another workspace; record workspace/focus behavior.
- [x] Accept tokenless urgency presentation as compositor-dependent behavior; the app issues the attention fallback.

Workspace movement, workspace switching, focus granting, and urgency presentation remain compositor policy. Observed
behavior is evidence for the tested compositor only, not a portable guarantee.

## Live observation — 2026-08-02

In the available Hyprland/Wayland session, the first direct launch owned `org.holonight.Settings` and created one mapped
window with class `org.holonight.Settings` on workspace 6. A second direct launch returned success and `pgrep` still
reported only the original PID. Runtime `busctl introspect` reported `Activate(a{sv})`, `Open(as,a{sv})`, and
`ActivateAction(s,av,a{sv})`. The existing window remained mapped on workspace 6; no workspace movement was requested.

After installation and enabling Hyprland's per-application `focus_on_activate` policy, the user confirmed that repeated
key-binding launches retain one process and reveal the existing settings window. Workspace switching and urgency
presentation remain compositor policy rather than portable application guarantees.

## Activation-token correction — 2026-08-02

The existing-process path now applies forwarded XDG activation tokens directly to the Qt Wayland surface instead of
temporarily restoring the startup environment variable. A live `WAYLAND_DEBUG=client` run on Hyprland 0.56.1 observed
an `xdg_activation_v1.activate(..., wl_surface)` request and the existing window receiving keyboard focus after a
repeat compositor launch with `focus_on_activate` enabled.
