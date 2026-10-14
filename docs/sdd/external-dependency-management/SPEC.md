# External dependency management — holonight-settings

Work package: EDM-013
Baseline: `6ec7a402ebc319f9a925fdf97d8e0382e0c1ef42`
Umbrella audit: `38146d8f474317f3f8d72699f60e6d16a71dd165`
Status: Verified locally; publication and pinning pending.

## Requirements

- Configuration and normal build commands shall never acquire dependencies or prepare source providers.
- `task deps` shall explicitly prepare supplied local providers and check installed external prerequisites.
- Internal providers shall resolve only from approved prefixes; external packages shall resolve from an explicit
  external prefix or normal system locations.
- Enabled dependency-dependent features shall require their dependencies; minimal profiles shall disable them explicitly.
- Where production components use Qt, discovery shall require >=6.11; private Qt components shall match
  the selected public version exactly.
- Where tests use GTest, consumers shall require installed GTest >=1.14 CONFIG and any GMock targets they use.
- Existing public APIs, component switches, install-tree consumers and acceptance coverage shall be preserved.

## Design

Adopt canonical `tooling/modules/holonight-settings.json` and `.cmake`, then synchronize bundle 2.0.0 into this repository.
The copied metadata is owned by this standalone checkout. CMake performs authoritative discovery; shared probes
provide prerequisite diagnostics and record resolved versions/paths. Incompatible caches and local provider install
trees are archived before reuse. Package acquisition occurs only in explicit environment preparation. CI switches
build/runtime/release lanes together to qualified immutable images and runs acceptance offline after preparation.

Preserve Shell configuration export consumption; keep BUILD_APP as the existing application switch.

## Implementation and verification tasks

- [x] Confirm the exact baseline and canonical handoff before implementation.
- [x] Adopt declarations, prefix helpers, explicit preparation and compatibility signatures.
- [x] Apply the repository-specific discovery changes above.
- [x] Update Task, CI preparation/entrypoints, Dockerfiles, release lanes and contributor instructions.
- [x] Run declaration/CMake/profile consistency checks and offline missing-package regressions.
- [x] Run narrow regressions, clean acceptance, installed-package consumers and required quality checks.
- [x] Inspect complete logs; record commands, versions, features, warnings and manual checks.
- [x] Make a repository-local commit only after local verification; publish only when authorized.

Implementation and automated local acceptance are complete; publication and umbrella pinning remain separate handoffs.

## Accepted implementation handoff — 2026-10-14

Umbrella checkpoint: `ca8f7b829f85ca65f76c6061aadb3a362cbefaeb`.
Implementation baseline: `b99c2fcc97a2d7e7c6f6f48f087cdb9fd1536452`, verified against canonical origin/main.
The original audit baseline above is retained. Provider revisions are fixed in scripts/ci/run.py and match
that checkpoint: Config, System Services, Qt, then Shell standalone config. Audio and Wayland stay enabled;
provider tests and demos are disabled. Installed exports require libpulse and toml++.

Acceptance matrix: Debug/tests (all existing CTest registrations, disposable buses, Controls and startup),
clean Release/staging (no test payload, four installed startup modes, no preparation-QML fallback),
strict format/tidy including separate package consumer and QML lint/import/metadata, and pinned REUSE.
All lanes run offline as caller UID/GID with read-only inputs and private runtime directories; provider
sources are unavailable to consumers. Focused launcher/discovery/cache regressions run first, including
BUILD_APP OFF with tests ON/OFF and coverage configuration. Automated gates control local completion;
fresh user-operated desktop review is deferred to EDM-022. Publication and pinning are separate handoffs.

## Local acceptance — 2026-10-14

All four qualified offline lanes passed in `build/ci/20261014T113152Z-wf6_19l8`.
Final Debug follow-up `build/ci/20261014T113716Z-twdrd0sz` passed with the additional Audio page-selection
assertion and corrected coverage configuration. Exact image IDs, source revisions/content/options,
compiler/environment evidence, input hashes, lane results and complete log hashes are in
[VERIFICATION.json](VERIFICATION.json). Logs remain under the recorded ignored build paths.

Commands and acceptance:

| Check | Command / result |
|---|---|
| Explicit acquisition/preparation | `JOBS=8 python3 scripts/ci/run.py --prepare --provider-source-root ..` (the root was supplied as the absolute umbrella path); clean provider build passed, then compatible provider reuse passed after launcher/coverage corrections |
| Four offline lanes | `JOBS=8 python3 scripts/ci/run.py`; Debug, Release, static/QML and licensing all passed |
| Final Debug | `JOBS=8 python3 scripts/ci/run.py --lane build-test`; all 86 CTest registrations passed with no skips, including disposable-bus tests, Controls styles, four startup modes, both wallpaper opening orders and activation-prefix checks |
| Installed consumers | Existing CTest consumer plus separately configured/built/executed `tests/package-consumer`; consumers receive only the installed prefix, with all provider sources/build trees unavailable |
| Release | `task configure PRESET=release`, configure-time install prefix `/work/settings-install`, parallel build/install; all four staged startup modes pass and explicitly forbid preparation QML paths; payload excludes test executables |
| Strict static | Explicit combined `build/test` and standalone `build/package-consumer` compiler databases; canonical scoped coverage validation, all 28 owned sources analyzed once per invocation with owned-header filtering; format, QML lint/import/metadata pass |
| Updated assertion | Focused qualified-image `clang-tidy tests/wallpaper_acceptance.cpp -p build/test --config-file=tests/.clang-tidy` with the canonical owned-header filter; final Debug compiler database, strict pass; qualified `clang-format --dry-run --Werror` pass |
| Regressions | 12 launcher, 16 real-CMake, 2 tidy-context, 11 canonical tooling and 21 canonical external fixtures pass (32 canonical total) |
| Minimal/coverage | BUILD_APP OFF with tests ON and OFF configure/build successfully; tests-ON runs applicable CTest entries. Tests OFF accepts disabled GTest discovery. Coverage config requires tests and discovers gcovr before generating the command; generated Ninja rule contains `gcovr --root` |
| Metadata and syntax | Canonical JSON/CMake equality, `python3 tooling/sync.py --module holonight-settings --check`, `python3 tooling/generate-install-profile.py --check` from umbrella, Python AST parsing, `sh -n` for both scripts, `task --list-all`, and `git diff --check` pass |
| Licensing | Pinned REUSE lane passes; historical supplement `packages.json` and disabled custom-image publication workflow are retained |

Providers use Debug with tests/demos disabled. System Services retains Audio, Storage, Compositor and Wayland;
Qt retains Wayland and existing syntax-theme defaults. Shell prepares only `libs/holonight-shell-config`.
The qualified image contains Qt 6.11.2, GCC 16.2.1, CMake 4.4.3, Ninja 1.13.2, Clang 22.1.8,
GTest 1.18.0, libpulse 17.0 and toml++ 3.4.0. Full package evidence is hashed in the verification record.
All acceptance containers use `--pull=never --network=none`, caller UID/GID, private mode-0700 runtime directories,
read-only source and installed-provider inputs. Launcher fixtures verify missing/stale/tampered preparations fail;
real-CMake fixtures verify missing/incompatible Qt and GTest/GMock, missing Audio, prefix conflicts/isolation/spaces,
compatible cache reuse and preserved incompatible caches. Normal configure/build/test never prepare or acquire sources.

Complete preparation/build/test/static/licensing logs were reviewed. No actionable compiler, QML or tidy warning
remains. Retained diagnostics are Qt's expected private-module compatibility notices, optional missing Vulkan
headers, existing suppressed external-header/NOLINT tidy counts, and expected offscreen/PulseAudio messages.
Negative launcher fixtures intentionally print failed mock lanes; their enclosing test suites pass. There are no
retained CTest skips. The initial missing-qmllint-target, empty wallpaper details and typed page-activation failures
were corrected and their affected checks rerun. Coverage's missing Task alias and late gcovr discovery were also
corrected and regression-tested. Release/static evidence was reused for unaffected code; the later test assertion
has final Debug, focused format and strict tidy evidence.

Separate cleanup commit `b82b14f` restores typed page activation and avoids opening an empty image filename,
with an observable Audio selection assertion. The dependency adoption commit follows separately.
No system installation, application restart, publication or submodule pin update was performed. Fresh user-operated
desktop review remains EDM-022; the initiative stays Accepted.

## Publication correction — 2026-10-14

The initial verified cleanup/adoption commits were published to canonical origin/main at
`49728a88c34b1edb211dbfb2c31d64c51f5d4118`. The latest CI query returned completed/failure
[Licensing run 38565122031](https://github.com/lebedenko/holonight-settings/actions/runs/38565122031).
Failure logs show the fresh runner lacked the pinned REUSE image while the acceptance launcher
correctly used `--pull=never`. Add an explicit acquisition step in the Licensing workflow before
the offline lane. This does not change application code, prepared providers or acceptance isolation.
Local verification passed the same pinned-image pull and offline licensing command in
`build/ci/20261014T123741Z-kc7tiip3`; complete log reviewed, REUSE reports zero errors. Workflow YAML
parsing, workflow/launcher image equality and whitespace checks pass. Unchanged
application acceptance evidence above is retained. Publication/pin records in the umbrella ledger
supersede the historical pending-publication status above.
