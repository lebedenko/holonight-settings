# Configuration architecture: holonight-settings

Work package: CA-005. Upstream baseline: `9829718cc01a4aab5c378493b9228406954cc143`.

## Scope

Pending edits, per-value conflicts, resets and guarded rollback.

Follow the accepted [shared contract](../../../../docs/initiatives/configuration-architecture/README.md).
Keep this repository independently buildable; do not modify another repository in its implementation commit.

## Design and acceptance

Each application owns its configuration schema, file, settings UI and behavior. Global appearance is shared;
application preferences are separate. Files and Viewer must remain usable without Shell or Settings. AI and Packages
retain their own configuration. Infrastructure requires neither Shell, Settings nor a running daemon.

Snapshots retain original bytes, parsed typed values, override presence, source spans and content revisions.
Paths are vectors of key segments, including quoted keys containing dots. Schemas declare typed defaults,
constraints, descriptions and reload policy, with domain validators for related values and dynamic collections.
Edit batches carry set/remove operations and baseline values/presence. Save outcomes distinguish success,
per-key conflicts, invalid documents/edits, unsupported patches, pre-replacement storage failures and
post-replacement durability failures.

Use toml++ and a TOML-aware lexical editor; never serialize an existing document wholesale or substitute by regex.
Preserve unrelated bytes, comments, ordering, whitespace and unknown fields. Reset removes an assignment and retains
comments/sections. Arrays and arrays of tables are whole conflict values. Unsupported safe patches fail unchanged.
Reparse and validate every candidate. Establish preservation fixtures before consumer adoption.

Merge baseline, pending and current values: unrelated edits merge, identical edits converge, different changes to the
same value conflict. Lock a stable sibling file for cooperating writers; read/patch under the lock and recheck the
revision immediately before replacement. Arbitrary editors do not participate in the lock: a race remains between
the final check and rename. Follow existing symlinks, abort on retargeting, preserve existing permissions, create new
files as 0600, sync a same-directory temporary file, rename, and sync the directory.

Appearance v2 uses sparse defaults, rejects invalid known fields and warns about preserved unknown fields. Retain the
v1 decoder and explicit v1 serialization APIs. Document version is metadata, separate from the effective appearance
model. First successful Settings save upgrades valid v1 by changing only version and requested values. New editing
documents use v2; unsupported versions are read-only. Enable GUI v2 writes only after readers/adapters pass compatibility.

Shell owns defaults/validation in its exported configuration package. Preserve paths and meanings. Reads never create
files or write defaults. Missing overrides use defaults; reset removes the override. Reject invalid known values.

Settings retains Save/Discard, tracks baseline/pending edits, refreshes untouched controls on external changes and
retains pending edits. Expose baseline/disk/pending values and per-value keep-pending/accept-external resolution;
recheck on save. Show default/override status and diagnostics. Discard loads latest disk. Invalid external documents
block saves and running consumers retain last valid values; startup errors use defaults with diagnostics. Missing
files use defaults without writes. Watch files and nearest existing parents through replacement/deletion/recreation;
publish only differing effective values. Domain saves have independent outcomes. Rollback is conditional on the staged
revision still being current; concurrent changes survive and must not be reported successfully applied.

- [ ] Preservation fixtures cover comments, unknown fields, quoted/dotted keys, inline tables, multiline strings, Unicode, CRLF, arrays/AoT, insertion/reset and rejected patches.
- [ ] Merge, convergence, conflicts/reset, cooperating locks and revision-change aborts pass.
- [ ] Unreadable files, permissions, interrupted writes, replacement failures, symlink retargeting and durability outcomes pass.
- [ ] v1 behavior remains compatible; sparse v2/reset and surgical first-save upgrades pass; unsupported versions cannot be overwritten.
- [ ] Runtime invalid/startup/missing/delete/recreate and unchanged-signal scenarios pass.
- [ ] Settings Save/Discard, external updates, per-value resolution, partial saves and adapter/rollback concurrency pass.
- [ ] Each repository passes required clean acceptance and installed-package checks at accepted provider revisions.
- [ ] Files and Viewer pass standalone checks without Shell or Settings installed.
- [ ] Every participating submodule is clean and pinned to a canonical published implementation commit.
- [ ] Dependency-order integration and user-operated concurrent-edit/appearance checks are recorded with dates and revisions.

## Implementation decisions

Providers consumed before GUI v2 writes: Config `d6a392b41991f70a004d58f7694c7b6115cb7280`, Qt
`98803bca05e16ae0d0784a6cb43b0ace561385de`, Shell schema `e490ff73f3da4b3a671aaf0496c8dbdc94a53ff8`,
and adapters `95a9078e8d4f0b2d9c5a69ee6040388558eb9251`. All were locally accepted and published before adoption.
System Services remains `398804a7cce5a57f9f6870c4e7ec99e9b1f3ddaa`.

`DocumentEditSession` owns a sparse edit batch with original presence and baseline values. Each domain projects
validated snapshots into its existing typed edit model; only differing properties are assigned. Preview patches
reuse the neutral lexical editor and never touch disk. Optional appearance extents track their enabled flags as
absence. Default resets remain dirty even when the effective value is unchanged. Shell controls use schema choices
and bounds, including its existing Kelvin preference; arbitrary valid refresh intervals remain visible.

File services subscribe to the reusable Qt watcher. Invalid documents retain the last valid controls and drafts.
Warnings are visible; unsupported versions and unreadable documents block writes. Conflicts retain original
baselines until a selected value is accepted or explicitly rebased. Resolution and subsequent saves reread disk.
Existing Appearance, Bar and Weather pages expose default/override state and resets; no other application page
is added. Conflict credentials are masked.

Appearance staging captures the exact pre-write snapshot under the writer lock and suspends watcher reconciliation
during adapter application. Commit checks physical destination and staged content before accepting saved edits.
Rollback uses the provider's physical-target/content guard. Native failure keeps a retry requirement even if an
external writer converged to the pending bytes. Edits made during application survive and rebase to the staged
snapshot after successful commit. Shell and Appearance outcomes are reported individually, including partial success.

Tooling builds the exported Shell configuration package from its repository subdirectory, preserving root revision
tracking without installing the Shell executable. System Services precedes Qt in local and clean provider builds.
