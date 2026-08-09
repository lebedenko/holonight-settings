# CTV-102: Settings appearance orchestration

Status: Implemented

Baseline: `holonight-settings@a960f2e`, `holonight-appearance-adapters@e3b3efd`,
`holonight-config@5cd36ec`, and `holonight-qt@e30ff79`.

## Adapter protocol

Settings invokes `holonight-appearance-adapter` asynchronously from an injected executable or `PATH`. Apply uses
`apply --appearance PATH --json`; integration actions use `status --json` and `revert --json`. Requests have a
15-second deadline and a 1 MiB stdout limit. Only protocol version 1, the requested operation, consistent
`result`/`success`/`degraded` values, a JSON outputs array, known output statuses, and the accepted `live`, `relaunch`,
`session-restart`, and `delegated` modes are accepted.

An adapter absent from `PATH` is degraded success so the canonical HoloNight setting remains useful without optional
native propagation. A resolved executable that cannot start, timeout, crash, non-conforming JSON, inconsistent exit
status, or excess output is a hard error. User-visible process diagnostics are fixed messages: canonical contents,
arguments, stderr, environment values, and filesystem paths are never copied into them.

## Appearance transaction

The existing neutral validation, Qt resolution, and external revision checks run before staging. Staging captures the
exact prior canonical bytes, existence state, and permissions, then atomically writes the candidate. The edit model is
not marked saved until apply reports success or degraded success.

On a hard adapter error, Settings atomically restores the captured file and its permissions, or removes the candidate
when the file did not previously exist. It then records the restored revision and leaves the model dirty. Staged state
rejects re-entry and confirmed overwrite follows the same transaction. The synchronous `save()` API remains available
for existing non-orchestrated callers and commits immediately.

Shell settings are a separate domain. A dirty Shell model is saved synchronously even if appearance validation,
conflict detection, staging, or adapter application fails. The aggregate result is finalized after asynchronous apply;
subsequent saves retry only models that remain dirty.

## Integrations UI

The Integrations page lists each output's stable name, status, apply mode, and concise adapter diagnostic. It provides
refresh/status, reapply, and restore-native-defaults actions. Actions are disabled during save or adapter activity;
reapply and revert are also disabled while appearance edits are dirty. Both the page and footer report degraded
propagation, rollback failures, conflicts, and relaunch or session-restart modes without exposing process details.

Revert preserves adapter compare-and-swap conflicts: Settings reports the returned output records and does not modify
the canonical HoloNight appearance. Refresh is read-only. Reapply uses the last committed canonical file.
