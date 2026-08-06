# Settings Single-Instance Activation — Tasks

**Status:** Implemented and verified

## Documentation and metadata

- [x] T-001 Write EARS requirements and independently testable criteria. (REQ-F-001–008, REQ-NF-001–002, REQ-C-001–003)
  - Check: every requirement has an Acceptance paragraph and compositor policy is explicit.
- [x] T-002 Record architecture, public interface, failure paths, extraction boundary, and alternatives. (REQ-NF-001–002, REQ-C-001–002)
  - Check: `DESIGN.md` names all interfaces and four rejected alternatives.
- [x] T-003 Rename/install desktop metadata and add matching D-Bus service metadata. (REQ-F-008)
  - Check: configured metadata has matching `org.holonight.Settings` identity and executable.

## Activation transport

- [x] T-004 Add app-local ownership arbitration and fail-closed forwarding. (REQ-F-001–003, REQ-F-007, REQ-C-001)
  - Check: independent bus connections resolve primary/secondary and introspection succeeds.
- [x] T-005 Export all `org.freedesktop.Application` methods and normalize them to activation. (REQ-C-001)
  - Check: meta-object and runtime introspection tests list all methods.

## Application lifecycle

- [x] T-006 Arbitrate before constructing configuration or QML state. (REQ-F-001, REQ-F-007)
  - Check: constructor returns on secondary/error before model allocation and load.
- [x] T-007 Exit successful secondary launches and fail startup errors. (REQ-F-002, REQ-F-007)
  - Check: `main.cpp` bypasses the event loop using the recorded startup result.

## Window activation

- [x] T-008 Queue activation until the root window is ready. (REQ-F-005)
  - Check: focused unit test activates before `setWindow()`.
- [x] T-009 Restore, raise, request activation, and add delayed attention fallback. (REQ-F-004, REQ-F-006, REQ-C-002)
  - Check: hidden-window test passes; source inspection confirms guarded delayed alert.

## Automated tests

- [x] T-010 Test tokens, queueing, arbitration, forwarding, errors, introspection, and metadata. (REQ-F-001–008, REQ-C-001)
  - Check: focused settings test suite and CMake metadata checks pass.
- [x] T-011 Run full automated regression and formatting checks. (REQ-NF-001–002)
  - Check: results are recorded in `VERIFICATION.md`.

## Live verification

- [x] T-012 Verify desktop-entry/direct repeat launches, minimized and other-workspace windows, tokenless launches, and
  one-process enforcement under live Wayland. (REQ-F-001–004, REQ-C-002)
  - Check: installed key-binding and direct-launch activation evidence is recorded; minimized restoration is covered
    automatically, while tokenless urgency presentation remains an accepted compositor-dependent limitation.

## Closure

- [x] T-013 Review documentation for renamed desktop ID and service. (REQ-F-008, REQ-C-003)
  - Check: current README/install references are accurate and old SDD files have no diff.
- [x] T-014 Close live verification. (REQ-C-002)
  - Check: T-012 is checked and accepted limitations are recorded.
