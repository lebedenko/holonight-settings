# DESIGN: Canonical Appearance Writer Adoption

## Domain decomposition

The current `SettingsEditModel` and `ConfigFileService` aggregate unrelated persisted authorities and implement a
three-file best-effort transaction. The target structure is:

```text
AppearanceEditModel <-> AppearanceFileService -> HoloNight::Config -> appearance.toml

ShellSettingsEditModel <-> ShellConfigFileService
                    -> HoloNightShellConfig::Config -> config.toml

SettingsSaveCoordinator -> invokes only dirty services and aggregates results for the UI
```

The coordinator contains no serialization and does not combine snapshots. It reports per-domain state so a partial
success is visible and retryable. This is a small application-level orchestration object, not another configuration
framework.

## Appearance edit model

`AppearanceEditModel` wraps the provider value and saved snapshot. Qt-friendly properties convert between
`std::string`/`std::optional<double>` and `QString`/enabled-value UI pairs at the boundary. Equality of provider values
drives dirty state; setters apply UI-safe clamping for interaction but the complete provider validation still runs on
save.

Catalog-derived properties such as dark/light and available accents are projections. Scheme changes normalize the
accent only through an explicit catalog rule visible to the user; arbitrary invalid values become validation errors,
not silent fallback. The editor stores no derived mode.

External-change protection records a lightweight file identity after successful load/save, such as provider-supported
content digest or file metadata plus content verification. Before writing, the service compares the current file with
that identity. Missing-to-created and changed-file conflicts require an explicit overwrite decision. Tests inject the
identity/filesystem seam rather than racing real timestamps.

## Shell product handoff

The existing local library is removed only after `HoloNightShellConfig::Config` is published. Application includes,
CMake package discovery, tests, and Task workflows switch together. Settings never vendors the Shell package or keeps
forwarding headers because that would leave two apparent schema authorities.

Product and appearance services resolve their own paths through their owning packages. Neither reaches into the
other's value. The app may load both at startup to populate all pages, but a save request is routed strictly by dirty
domain and cannot cause an unrelated rewrite.

## Save UX

One Save action is preferred because the current Apply and Save & Apply buttons are identical. During a multi-domain
save, the coordinator disables repeat activation, invokes each dirty service, then presents a summary:

- all requested domains saved;
- appearance saved, Shell settings failed;
- Shell settings saved, appearance failed;
- both failed.

Successful services mark saved immediately. Retrying invokes only remaining dirty services. There is no compensating
write because each document is independently authoritative and atomic.

## Diagnostics and security

Provider error codes are mapped to concise localized messages with path and source position when safe. Raw document
contents, tokens, passwords, and authenticated query strings are never inserted into logs or dialogs. Shell product
diagnostics follow the redaction guarantees of its provider package.

## Trade-offs

- Separate models add a small amount of wiring but make ownership, dirty state, and failure recovery explicit.
- Optimistic external-change detection adds a confirmation path even with one supported writer, protecting manual
  edits and developer tools from silent loss.
- Removing duplicate Apply/Save labels reduces apparent functionality, but an honest single action is clearer than
  two controls with identical implementation.
