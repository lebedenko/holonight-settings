# Shared navigation preference

Settings Keyboard exposes Standard and Vim navigation for Files, chooser and
Bluetooth, keeping conventional text editing. Config owns the input schema and
path; Shell remains owner of its sections. The existing Settings Shell model is
the presentation adapter for the general preferences document, with a separate
navigation property projected through Config rather than Shell ProductConfig.

Compose Config input fields with the installed Shell schema in a single
DocumentEditSession. This preserves comments and unknown keys, merges unrelated
external edits, reports same-key conflicts, and shares Save/Discard/reset behavior.
Qt InputPreferences reads independently and exposes diagnostics while retaining
last valid state. Missing keys and deletion select Standard; reading never writes.

Acceptance: decoder defaults/validation, live replacement/deletion, combined Shell
and input saves, preservation, reset, discard and external conflicts. Consumer
profile routing and text/modal isolation keep the existing shared interpreter.
Manual native acceptance requires matching installed providers and consumers.
