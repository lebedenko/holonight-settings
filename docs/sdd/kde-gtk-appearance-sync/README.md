# Native appearance status

Settings keeps the existing staged save, adapter apply, and rollback transaction. The Integrations page sends the current canonical appearance path with status refresh, so the adapter can distinguish output edits from canonical changes. Reapply continues through the same adapter client.

Verification: focused `AppearanceAdapterClientTest` and `SettingsSaveCoordinatorTest`, then repository build, test, formatting, tidy, QML lint, QML types, and import checks.
