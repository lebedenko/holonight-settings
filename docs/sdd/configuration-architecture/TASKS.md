# CA-005 implementation

- [x] Inspect existing implementation and preserve public contracts.
- [x] Implement repository-owned scope from README.
- [x] Add focused behavioral regression coverage.
- [x] Run focused checks, clean acceptance and installed consumer checks.
- [ ] Review final diff, commit, publish and hand off exact revision.

2026-10-06 local: all 30 focused document/file-service/coordinator regressions passed. The broader 55-case unit binary passed 53 cases with two private-bus cases skipped outside the dedicated isolated workflow. Clean acceptance and final QML/static checks remain pending; these focused results do not imply integration.

2026-10-06: clean `task ci` passed build-test, static-checks and licensing (`build/ci/20261006T084611Z-p1s1mvx_/`). All 67 registrations passed in private D-Bus, including default/Fusion control windows, four build startup modes and four installed startup modes. Complete clean logs reviewed; no actionable compiler/static diagnostics. Installed consumer analysis passed. Local affected follow-up: 9 coordinator/window checks, QML lint without diagnostics, formatting, test/debug type metadata and import policy passed. Source/test analysis passed after focused corrections. Provider revisions are recorded in README and the CI lane. Publication and umbrella/manual ecosystem acceptance remain separate.
