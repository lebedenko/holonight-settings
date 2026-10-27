# Compositor-aware Settings

Settings presents applicable controls while keeping one shared configuration document.
The app-owned `SettingsSessionContext` calls the installed
`HoloNightSystem::Compositor` selector once at startup. It exposes a constant compositor
identity and `isApplicable(property)` to QML without creating or starting a backend.
Desktop tokens take precedence over stale environment markers; conflicting evidence or
missing identity selects common controls only.

The window passes this context through ContentStack to BarPage. Visible workspace count
is available on Hyprland, Sway and labwc, and means the maximum number of labels shown
in the bar. Taskbar, grouping, overview and desktop menu are labwc-only. Other fields
are common. Both controls and override rows use the same lookup. Hiding the window
management group removes its header and layout spacing.

Applicability is presentation only: document loading, editing, schema and saving retain
all fields. No migration, per-compositor files, provider API or Shell plugin is needed.
Session identity remains stable through IPC outages. Navigation placeholders are unchanged.

Acceptance covers supported and unknown sessions, conflicting markers and desktop tokens,
stale-marker precedence, UI visibility, override filtering, collapsed section geometry,
and preservation of hidden values and unrelated bytes when a common setting is saved.
Native builds consume installed dependencies; QML lint, import policy, generated type
metadata and formatting checks cover integration.
