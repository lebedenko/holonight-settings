# Theme and Typography Configuration Architecture

## Current state

HoloNight currently spreads theme-related settings across three files:

- `$XDG_CONFIG_HOME/holonight/theme.conf` is owned by `holonight-qt` and contains the runtime color scheme, accent, icons, effects, scale, and typography.
- `$XDG_CONFIG_HOME/holonight/config.toml` is owned by the shell configuration package. It still contains transitional typography fields consumed by the shell.
- `$XDG_CONFIG_HOME/holonight/appearance.json` is owned by `holonight-qt` and contains shape configuration only.

Before this change, `holonight-settings` populated its typography controls from `config.toml`, while Qt controls populated their typography from `theme.conf`. Applying a font change therefore updated shell-owned values without updating already-created HoloNight controls. Settings also rewrote `theme.conf` with only the appearance group, discarding unrelated theme data.

`holonight-qt` additionally recognized a legacy `theme.json` file and selected it when `theme.conf` was absent. That created another possible source of runtime values and made the effective source depend on which files happened to exist.

## Ownership defects

1. Typography has more than one writable authority.
2. Settings' displayed value can differ from the value used by `HnLabel` and other shared controls.
3. Applying Settings cannot notify existing QML typography bindings because `HolonightTheme` properties are constant.
4. Rewriting only the known appearance keys loses advanced fonts, icons, effects, scale, unknown sections, and unknown keys.
5. Legacy JSON fallback makes file discovery ambiguous and prevents `theme.conf` from being authoritative.

## Target data flow

`theme.conf` is the only theme and typography file recognized by `holonight-qt` and `holonight-settings`:

```text
theme.conf [appearance] -> palette resolver -> HoloniightPalette
theme.conf [fonts]      -> theme config     -> HolonightTheme -> HnLabel and shared controls
theme.conf other groups -> shared readers   -> icons/effects/scale
appearance.json         -> HnAppearance (shape only)
config.toml             -> shell configuration (transitional typography preserved, ignored by Settings)
```

Environment variables remain higher-precedence runtime overrides. The shared API exposes file values separately so Settings always edits persisted values rather than copying an environment override into the file.

Apply saves `config.toml`, then a preserving `theme.conf` update, then `appearance.json`. The edit model is marked clean only after every configured write succeeds. A successful Apply reloads `HolonightTheme`, `HoloniightPalette`, and `HnAppearance`; an unsuccessful theme write reports the `theme.conf` path and leaves the model dirty.

## File disposition

| File | Disposition |
|---|---|
| `theme.conf` | Sole theme and typography source. Settings owns `[appearance]` scheme/accent/mode and `[fonts]` ui/fixed/baseSize/fixedSize, preserving all other content. |
| `appearance.json` | Shape configuration only. |
| `config.toml` | Shell-owned configuration. Existing typography keys remain source-compatible and are preserved during unrelated Settings saves, but Settings neither displays nor updates them. |
| `theme.json` | Unsupported and ignored. Existing user files are not deleted. It has no compatibility or migration role. |

## Staged rollout

1. Consolidate `theme.conf` parsing, preservation, and path resolution in `holonight-qt`.
2. Make `HolonightTheme` reloadable and notify existing QML bindings, including watcher re-arming after atomic replacement or directory creation.
3. Move Settings typography state to its own `theme.conf` snapshot and reload shared runtime singletons after Apply.
4. Keep the current shell typography fields untouched during this milestone.
5. Migrate shell typography consumers and remove their transitional TOML fields in a later shell-repository phase.
