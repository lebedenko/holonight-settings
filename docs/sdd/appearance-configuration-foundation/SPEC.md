# SPEC: Canonical Appearance Writer Adoption

**Initiative:** ACF-005
**Date:** 2026-08-07
**Status:** In Progress
**Repository baseline:** `73f3d15033d48be637c6a6411fb76259c29b3b1a`
**Umbrella contract:** `3ac1306`
**Appearance provider:** `holonight-config` `81b01d3`
**Qt provider:** `holonight-qt` `6f591cb`
**Shell product provider:** `holonight-shell` `93e1faf`

## Goal

Make HoloNight Settings the only production writer of canonical global appearance while turning it into a consumer,
not owner, of Shell product configuration. Replace the current three-file sequential save with explicit independent
appearance and Shell-settings domains.

## Dependencies and ownership

- Consume `HoloNight::Config` for the appearance schema, exact defaults/ranges, path resolution, parsing,
  serialization, atomic writing, diagnostics, and test helpers.
- Consume the accepted `holonight-qt` package for theme-family/scheme/accent catalog data and Qt/QML design-system
  controls. Settings performs catalog validation but does not duplicate catalog tables.
- Consume `HoloNightShellConfig::Config` from the accepted Shell provider for Shell product settings.
- Delete the locally exported `libs/holonight-config`, `HolonightConfig::Config` package, and
  `holonight_config/` public headers after Shell publishes its replacement and all local call sites are migrated.
- Settings owns edit models, UI validation, dirty/saved snapshots, explicit save/discard commands, progress state, and
  user-visible diagnostics. It owns no persisted schema.

## Separate edit and save domains

Settings shall expose two independent domains:

1. `AppearanceEditModel` and `AppearanceFileService` for `appearance.toml`;
2. a Shell settings edit model and `ShellConfigFileService` for `config.toml`.

Each domain has its own current value, saved snapshot, dirty flag, busy flag, load result, and save result. A UI-level
save coordinator may present one action, but it invokes only dirty domains and never claims cross-file atomicity.

- An appearance-only save does not parse, create, or write `config.toml`.
- A Shell-settings-only save does not parse, create, or write `appearance.toml`.
- When both domains are dirty, each atomic write runs independently. Success advances only that domain's snapshot;
  failure leaves only that domain dirty and retryable.
- User-visible results identify the failed domain and provider diagnostic without exposing unrelated document
  contents.
- Discard reloads only the selected/requested dirty domains. A global Discard action may request both explicitly.
- Repeated save activation while the affected domain is busy is ignored or disabled.

The existing `ConfigFileService` three-step write to `config.toml`, `theme.conf`, and `appearance.json` is removed.
`ThemeConfigFile` is deleted.

## Appearance load and edit behavior

- Missing `appearance.toml` initializes `AppearanceEditModel` from `HoloNight::Config::defaults()` without creating a
  file or marking the model dirty.
- A present invalid document leaves the editor on shared defaults at first startup, reports the structured error, and
  does not overwrite the invalid file until the user explicitly saves.
- Discard reloads the complete persisted appearance. A failed discard/reload retains the current edit value and saved
  snapshot and reports the error.
- Save validates the complete neutral value, then validates scheme/accent against the pinned Qt catalog before calling
  the provider atomic writer.
- Only a successful write advances the appearance snapshot and clears appearance dirty state.
- The editor does not watch and silently merge external changes. If the file changes after load, saving must detect
  that the on-disk revision differs and ask the user to reload or explicitly overwrite; it must not silently clobber
  another writer despite Settings being the only supported production writer.

## Appearance UI contract

The Appearance page edits every canonical v1 selection:

- scheme and accent;
- UI, monospace, title, and display font families and sizes;
- icon theme, fallback icon theme, and cursor theme;
- layout scale;
- shape style, scale, optional base radius, and optional base chamfer.

The dark/light control is a catalog projection: it selects the corresponding sibling scheme when one exists and is
never persisted as `mode`. If no sibling exists, the UI disables that transition or explains it; it must not silently
select an unrelated family.

Remove transparency and blur controls, properties, tests, and documentation. Rename fixed/header/clock terminology to
canonical monospace/title/display roles throughout the edit model and UI. Font controls accept installed system
families but retain and display a configured unavailable family so opening Settings does not silently mutate state.

Shape optional values use `std::optional` semantics from the provider, not NaN sentinels. UI slider ranges and steps
must be compatible with provider validation; the provider remains authoritative.

## Apply behavior

Writing the canonical document is the apply mechanism: Qt and Shell consumers observe its atomic replacement. The
UI must not call ad-hoc Shell reload APIs, directly modify portal state, or write legacy files.

If `Apply` and `Save & Apply` remain visually distinct, their behavior must be explicitly different and tested.
Otherwise consolidate them into one `Save` action; two labels invoking the same method are not retained as false
semantics.

## Clean-break and security requirements

After ACF-005, Settings does not read, write, watch, document, export, or test `theme.conf`, `appearance.json`, or
appearance/theme fields in Shell `config.toml`. It has no copied appearance or Shell-product defaults, TOML parser,
writer, legacy mode, transparency/blur fields, or field environment aliases.

Credential/private-URL migration is handled by the separate Shell security initiative. Until then, Settings may edit
Shell product fields through the Shell-owned package, but appearance-only actions cannot touch that domain. Tests and
diagnostics use redacted fixtures and never display or log secret values.

## Verification

- Unit-test appearance defaults, every field/range, catalog validation, derived mode UI, optional shapes, dirty
  tracking, missing/invalid load, save success/failure, discard failure, and external-change conflict handling.
- Test domain isolation: appearance-only actions do not open/write product config and product-only actions do not
  open/write appearance.
- Test two-domain partial success and retry without rolling back or resaving the successful domain.
- QML-test all canonical controls, removal of transparency/blur, unavailable-font retention, error presentation,
  button enablement, and honest Save semantics.
- Install/use the published appearance and Shell product packages; verify no local exported config package remains.
- Search production, tests, docs, CMake, and install manifests for legacy paths, schemas, APIs, fields, and aliases.
- Run format, tidy, QML lint/types, focused tests, complete CTest, and manual Settings-to-Qt/Shell live application
  checks under Hyprland.

## Non-goals

- Owning the global appearance or Shell product schema.
- Watching appearance for live preview or merging concurrent edits automatically.
- Credential migration or secret-store UI.
- Cross-toolkit adapter settings before the CTV initiative resumes.
