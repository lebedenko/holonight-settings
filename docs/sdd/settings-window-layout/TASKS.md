# SDD Tasks — settings-window-layout

Ordered for implementation dependency. Design resolutions from `DESIGN.md` (§4–§6) take precedence over SPEC.md prose where they conflict.

---

- [x] T-001: Create PreviewPanel.qml placeholder component
  - REQs: REQ-F-4.1, REQ-F-4.7, REQ-C-2.4
  - Check: File `apps/settings/qml/PreviewPanel/PreviewPanel.qml` exists with transparent `Item` root containing a centered `Text { text: "Preview" }` and imports `QtQuick`, `Holonight.Core`, `Holonight.Controls`; code inspection confirms zero service bindings, dynamic models, or feature-state references.

- [x] T-002: Register PreviewPanel in SETTINGS_QML_FILES
  - REQs: REQ-F-4.1 (compilation prerequisite)
  - Check: `apps/settings/CMakeLists.txt` lists `qml/PreviewPanel/PreviewPanel.qml` in `SETTINGS_QML_FILES`; `SettingsWindow.qml` imports `HolonightSettings` so `PreviewPanel { }` resolves in both runtime compilation and `qmllint`.

- [x] T-003: Make NavPanel.qml root color transparent
  - REQs: REQ-C-2.2, REQ-F-2.2, REQ-F-2.3
  - Check: `NavPanel.qml` root `Rectangle` has `color: "transparent"`; nav item list, order, page-switching logic, and internal styling remain unchanged; the list scrolls when constrained so every page remains reachable at the 480px minimum window height.

- [x] T-004: Restructure SettingsWindow.qml layout to three-column format
  - REQs: REQ-F-1.1, REQ-F-1.2, REQ-F-1.3, REQ-F-1.4, REQ-F-2.1, REQ-F-2.3, REQ-F-2.5, REQ-F-2.6, REQ-F-3.1, REQ-F-3.3, REQ-F-3.4, REQ-F-3.5, REQ-F-3.6, REQ-F-4.1, REQ-F-4.3, REQ-F-4.4, REQ-F-4.5, REQ-F-4.6, REQ-F-5.1, REQ-F-5.3, REQ-F-5.4, REQ-F-6.1, REQ-C-3.1, REQ-C-3.2
  - Check: Root `Rectangle` has `color: HoloniightPalette.background` (changed from `surface`); `ColumnLayout` with `anchors.fill: parent` and `spacing: 8` contains a `RowLayout` with 8px left/top/right margins and 8px spacing between three `HnSurfaceFrame` children (nav `Panel` with a 220px floor and title-derived preferred width plus `TopRight|BottomRight` chamfer, content `Window` with no `chamferedCornersOverride`, preview 320px `Panel` with `TopLeft|BottomLeft` chamfer). Nav and preview content have 8px inner margins. `FooterBar` is the second `ColumnLayout` child, fills the width, and uses `HoloniightPalette.surface`; `task qml-lint` passes without new warnings.

- [x] T-005: Update minimum window size in SettingsApplication.cpp
  - REQs: REQ-C-1.1, REQ-C-1.2, REQ-C-1.3
  - Check: `SettingsApplication.cpp` contains `view_->setMinimumSize(QSize(1244, 480))` and `view_->resize(1200, 800)` unchanged; window compositor rejects resize requests attempting width below 1244 or height below 480 pixels.

- [x] T-006: Build, lint, and verify complete layout against V1/V2 checklist
  - REQs: REQ-NF-1.1, REQ-NF-1.2, REQ-V-1, REQ-V-2
  - Check: `task build` and `task qml-lint` both pass without errors or new warnings; manual verification in live Hyprland session confirms: (V1.1) the window displays with a 220px-minimum, title-aware nav, fluid content, 320px preview, full-width footer, and approved 8px composition gaps; (V1.2) resize larger maintains side-column widths and expands content; (V1.3) resize toward 1244×480 succeeds, below minimum is rejected, and every navigation page remains reachable by scrolling; (V1.4) corner topology: nav left rounded+right chamfered, content top-left+bottom-right rounded+others chamfered, preview left chamfered+right rounded; (V1.5) colors: nav/preview `surfaceRaised`+`borderPassive`, content/footer `surface`, gaps show `background`; (V1.6) footer status+version left, buttons right; (V1.7) page navigation loads correct pages, preview visible on all; (V1.8) preview contains only "Preview" label, no widgets/bindings; (V2.1) qml-lint clean; (V2.2) build succeeds; (V2.3) QML smoke test instantiates the layout without crashes.

---

## Implementation Notes

- **T-001 & T-002**: Create PreviewPanel before building so CMakeLists registration resolves during configure.
- **T-003**: Change NavPanel color before structuring SettingsWindow (§4 of DESIGN.md explains why this is necessary for correct visual output of the wrapping frame).
- **T-004**: Use named-flag expressions like `HnCornerMask.TopLeft | HnCornerMask.BottomLeft` (not numeric literals) per §6 risk #5.
- **T-005**: Independent of QML changes; performed after layout is finalized to confirm sizing integration.
- **T-006**: Final rollup verifies both automated (build/lint) and manual (layout/topology/colors/interaction) requirements from V1 and V2.

---

## Document History

| Date | Author | Version | Notes |
|------|--------|---------|-------|
| 2026-07-30 | Claude Code | 1.0 | Task breakdown derived from SPEC.md v1.0 and DESIGN.md Stage 2; follows DESIGN.md resolutions on nav width (200px), preview chamfer mask (named flags, not numeric), NavPanel transparency edit, minimum size (1224×480). |
| 2026-07-30 | Codex | 1.1 | Align accepted checks with the approved 8px mockup geometry, footer surface token, and scrollable navigation at minimum height. |
