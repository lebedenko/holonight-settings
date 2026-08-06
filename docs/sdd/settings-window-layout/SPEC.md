# Settings Window Layout Refactoring — Specification

> **Authoritative window-host amendment:** `SettingsWindow.qml` is a top-level `HnApplicationWindow` loaded by
> `QQmlApplicationEngine::loadFromModule()`. QML owns the 1244×480 minimum and 1244×800 initial size, while the
> platform retains native compositor decoration. This supersedes older `QQuickView`, 1200px startup-width, and
> C++-owned geometry requirements below. The footer exposes `implicitHeight`, is sized by the parent layout, and
> uses `surfaceRaised` for the persistent action region.

**Scope**: Refactor the HoloNight Settings window layout from a 2-column (nav + content + footer) to a 3-column layout (nav + content + preview panel + full-width footer), using the shared `HnSurfaceFrame` component's semantic corner-shaping API for internal panel boundaries.

**Status**: Finalized (stakeholder grilling complete; decisions locked)

---

## Non-Goals

The following items are explicitly out of scope for this cycle and must NOT be addressed in implementation:

- Changes to settings controls beyond moving panel titles into fixed headers and standardizing deferred placeholders
- Changes to `NavPanel`'s internal nav item list, order, or behavior
- Functional content in the new right panel (preview system, live config mirrors, weather, now-playing, file browser, or any other feature integration)
- Changes to footer button behavior (Discard Changes, Apply, Save & Apply) or enablement logic
- Changes to the appearance/theme configuration system itself
- Reproduction of any native window glow, shadow, or compositor-provided frame effects
- Resizing of the current right sidebar/sidebar widget (the sidebar is a separate compositor-owned layer-shell surface; this spec covers only the settings window itself)

---

## Functional Requirements

### F1: Layout Architecture

**REQ-F-1.1 — Three-Column Root Layout Structure**

The root `SettingsWindow.qml` shall contain a `ColumnLayout` (or equivalent container with vertical stacking semantics) as its top-level layout, with `anchors.fill: parent` to occupy the entire window.

**Acceptance Criterion**:
- The QML code instantiates a `ColumnLayout` as a direct child of the root item; all three columns (nav, content, right) are children of a single horizontal (RowLayout-style) layout, and the footer is a separate child below the row in the ColumnLayout's vertical stack.

**REQ-F-1.2 — Horizontal Column Container**

The three columns (nav panel, main content, preview panel) shall be organized in a single `RowLayout` that is itself a child of the root `ColumnLayout`, with `Layout.fillHeight: true` and `Layout.fillWidth: true`.

**Acceptance Criterion**:
- A `RowLayout` wraps all three column instances; the row has `Layout.fillHeight: true` and `Layout.fillWidth: true` set on the QML Layouts attached properties.

**REQ-F-1.3 — Gap Between Window Edge and Columns**

Eight pixels of spacing shall be applied between the window edges (left, top, right) and the outer edges of the three-column row, matching the approved design mockup. The footer remains flush with the window edges, while its content remains inset by 16px.

**Acceptance Criterion**:
- Visual inspection: an 8px horizontal gap is visible between the left edge of the window and the left edge of the nav panel; an 8px gap exists between the right edge of the right panel and the window's right edge; an 8px vertical gap exists between the top of the row and the window top; the footer reaches the left, right, and bottom window edges; footer text/buttons are inset 16px from left and right edges.

**REQ-F-1.4 — Spacing Between Columns**

Eight pixels of spacing shall be applied between adjacent column boundaries: nav↔content and content↔right panel.

**Acceptance Criterion**:
- Visual inspection: an 8px gap is visible between the right edge of the nav panel frame and the left edge of the content frame; an 8px gap is visible between the right edge of the content frame and the left edge of the right panel frame.

---

### F2: Nav Panel (Left)

**REQ-F-2.1 — Nav Panel Frame Wrapping**

The `NavPanel.qml` component shall be wrapped in an `HnSurfaceFrame` with `surfaceRole: HnSurfaceRole.Panel` and `chamferedCornersOverride: HnCornerMask.TopRight | HnCornerMask.BottomRight`.

**Acceptance Criterion**:
- The QML instantiation includes `HnSurfaceFrame { surfaceRole: HnSurfaceRole.Panel; chamferedCornersOverride: HnCornerMask.TopRight | HnCornerMask.BottomRight; }` wrapping the NavPanel instance; the chamferedCornersOverride evaluates to value 6 (`0x0110` in binary: TopRight=2, BottomRight=4).

**REQ-F-2.2 — Nav Panel Corner Topology**

The nav panel's top-left and bottom-left corners shall be rounded (curved); the top-right and bottom-right corners shall be chamfered (diagonal straight cut).

**Acceptance Criterion**:
- Visual inspection at 100% zoom in a live Hyprland session: the nav panel's left-side corners are visually rounded; the right-side corners are chamfered (appear as diagonal cuts pointing inward toward the content area).

**REQ-F-2.3 — Nav Panel Fill Color**

The nav panel's `HnSurfaceFrame` shall set `fillColor: HoloniightPalette.surfaceRaised`.

**Acceptance Criterion**:
- The QML code includes `fillColor: HoloniightPalette.surfaceRaised` on the wrapping HnSurfaceFrame instance.

**REQ-F-2.4 — Nav Panel Border Color**

The nav panel's `HnSurfaceFrame` shall set `borderColor: HoloniightPalette.borderPassive`.

**Acceptance Criterion**:
- The QML code includes `borderColor: HoloniightPalette.borderPassive` on the wrapping HnSurfaceFrame instance.

**REQ-F-2.5 — Nav Panel Content-Aware Width**

The nav panel shall have a minimum width of 280 pixels. Its preferred width shall grow from that baseline when
the full, non-elided application title requires more space, but shall not stretch proportionally with the window.

**Acceptance Criterion**:
- `Layout.minimumWidth` on the nav frame is 280.
- `Layout.preferredWidth` is the greater of 280 and the title's full implicit width.
- `Layout.fillWidth: false` prevents unrelated window growth from stretching the navigation column.
- The application-title label is not elided at the resolved preferred width, including after its text or theme font changes.
- The branded header is aligned with the center and preview headers and does not display a divider.

**REQ-F-2.6 — Nav Panel Height Behavior**

The nav panel shall have `Layout.fillHeight: true` on the row, so it stretches to match the tallest column.

**Acceptance Criterion**:
- The NavPanel (or its wrapping HnSurfaceFrame) has `Layout.fillHeight: true` set; when the window is resized taller, the panel visually extends to the full row height.

---

### F3: Main Content Area (Center)

**REQ-F-3.1 — Content Frame Wrapping**

The `ContentStack.qml` component shall be wrapped in an `HnSurfaceFrame` with `surfaceRole: HnSurfaceRole.Window` and shall NOT set `chamferedCornersOverride` (leaving it at the role's default `Inherit` value).

**Acceptance Criterion**:
- The QML instantiation includes `HnSurfaceFrame { surfaceRole: HnSurfaceRole.Window; }` wrapping ContentStack; no `chamferedCornersOverride` is explicitly set.

**REQ-F-3.2 — Content Corner Topology (Hybrid Default)**

The main content area's corner topology shall follow `HnSurfaceRole.Window`'s default Hybrid mode: top-left and bottom-right corners shall be rounded; top-right and bottom-left corners shall be chamfered.

**Acceptance Criterion**:
- Visual inspection at 100% zoom: the content area's top-left and bottom-right corners appear rounded; top-right and bottom-left corners appear chamfered (diagonal cuts).

**REQ-F-3.3 — Content Fill Color**

The content area's `HnSurfaceFrame` shall set `fillColor: HoloniightPalette.surface`.

**Acceptance Criterion**:
- The QML code includes `fillColor: HoloniightPalette.surface` on the wrapping HnSurfaceFrame instance.

**REQ-F-3.4 — Content Border Color**

The content area's `HnSurfaceFrame` shall set `borderColor: HoloniightPalette.borderPassive`.

**Acceptance Criterion**:
- The QML code includes `borderColor: HoloniightPalette.borderPassive` on the wrapping HnSurfaceFrame instance.

**REQ-F-3.5 — Content Flexible Width**

The main content area shall have `Layout.fillWidth: true`, occupying all remaining horizontal space after the nav and right panels have taken their fixed allocations.

**Acceptance Criterion**:
- `Layout.fillWidth: true` is set on the content area's wrapping HnSurfaceFrame; when the window is resized horizontally, the content area expands/contracts while the two side panels remain fixed width.

**REQ-F-3.6 — Content Height Behavior**

The content area shall have `Layout.fillHeight: true` on the row, matching the tallest column.

**Acceptance Criterion**:
- The ContentStack (or its wrapping HnSurfaceFrame) has `Layout.fillHeight: true` set; visual verification confirms the frame extends to the full row height.

**REQ-F-3.7 — Content Loader Behavior Unchanged**

The `ContentStack` loader's page-switching behavior (selecting between `AppearancePage.qml`, `BarPage.qml`, etc. based on the `currentPage` property) shall remain unchanged from the current implementation.

**Acceptance Criterion**:
- Navigating between settings pages in the left nav panel still loads the correct page component in the content area; implemented controls remain unchanged after page-local headings move into the fixed content header.

---

### F4: Preview Panel (Right)

**REQ-F-4.1 — Preview Panel Frame Wrapping**

A new `PreviewPanel.qml` component shall be created and wrapped in an `HnSurfaceFrame` with `surfaceRole: HnSurfaceRole.Panel` and `chamferedCornersOverride: HnCornerMask.TopLeft | HnCornerMask.BottomLeft`.

**Acceptance Criterion**:
- A new file `apps/settings/qml/PreviewPanel/PreviewPanel.qml` exists; it is instantiated in `SettingsWindow.qml` wrapped in an `HnSurfaceFrame { surfaceRole: HnSurfaceRole.Panel; chamferedCornersOverride: HnCornerMask.TopLeft | HnCornerMask.BottomLeft; }`; the chamferedCornersOverride evaluates to value 3 (`0x0011` in binary: TopLeft=1, BottomLeft=8).

**REQ-F-4.2 — Preview Panel Corner Topology**

The preview panel's top-left and bottom-left corners shall be chamfered (diagonal straight cut); the top-right and bottom-right corners shall be rounded (curved).

**Acceptance Criterion**:
- Visual inspection at 100% zoom: the preview panel's left-side corners are chamfered (diagonal cuts pointing inward); the right-side corners are rounded.

**REQ-F-4.3 — Preview Panel Fill Color**

The preview panel's `HnSurfaceFrame` shall set `fillColor: HoloniightPalette.surfaceRaised`.

**Acceptance Criterion**:
- The QML code includes `fillColor: HoloniightPalette.surfaceRaised` on the wrapping HnSurfaceFrame instance.

**REQ-F-4.4 — Preview Panel Border Color**

The preview panel's `HnSurfaceFrame` shall set `borderColor: HoloniightPalette.borderPassive`.

**Acceptance Criterion**:
- The QML code includes `borderColor: HoloniightPalette.borderPassive` on the wrapping HnSurfaceFrame instance.

**REQ-F-4.5 — Preview Panel Fixed Width**

The preview panel shall have a fixed `Layout.preferredWidth` of 320 pixels (adjustable by ±50px if implementation review identifies a UX need, but must be fixed, not proportional).

**Acceptance Criterion**:
- `Layout.preferredWidth` on the preview panel instance is set to a value >= 270 and <= 370; `Layout.fillWidth: false` ensures it does not stretch.

**REQ-F-4.6 — Preview Panel Height Behavior**

The preview panel shall have `Layout.fillHeight: true` on the row, stretching to the full height of the three-column row.

**Acceptance Criterion**:
- The PreviewPanel (or its wrapping HnSurfaceFrame) has `Layout.fillHeight: true`; visual verification confirms it extends to the full row height.

**REQ-F-4.7 — Preview Panel Header and Placeholder Content**

The `PreviewPanel.qml` root item shall contain a fixed-height `HnHeaderBar` titled "Preview" and a body label displaying "Not yet implemented", with no functional content or data bindings to app state.

**Acceptance Criterion**:
- The header remains visible at the top of the panel, the placeholder is centered in the remaining body, and a code review confirms no data properties are bound to services, models, or app state.

**REQ-F-4.8 — Preview Panel Always Visible**

The preview panel shall be visible on every settings page (no per-page `visible: false` or `Layout.fillHeight: false` toggling).

**Acceptance Criterion**:
- The preview panel has no conditional `visible` property binding; `visible` is always `true` (or implicitly true by default); navigating between all pages confirms the panel remains visible on every page.

---

### F5: Footer

**REQ-F-5.1 — Footer Full-Width Repositioning**

The `FooterBar.qml` component shall be repositioned in the layout hierarchy as a direct child of the root `ColumnLayout` (below the three-column row), with `Layout.fillWidth: true`, so it spans the full window width.

**Acceptance Criterion**:
- The ColumnLayout children are: (1) the horizontal RowLayout containing the three columns, and (2) the FooterBar as the second child; FooterBar has `Layout.fillWidth: true` set.

**REQ-F-5.2 — Footer Below All Columns**

The footer shall visually appear below all three columns, with no part of the footer overlapping or nested inside any column.

**Acceptance Criterion**:
- Visual inspection: the footer's top edge is below the bottom edge of all three column frames; the footer is not clipped by any parent layout of the columns.

**REQ-F-5.3 — Footer Internal Content Alignment Preserved**

The footer's internal `HnActionBar` shall retain its current layout: leading content (status indicator + version label) left-aligned; trailing content (three buttons) right-aligned. This alignment must not be redesigned.

**Acceptance Criterion**:
- Visual inspection: the status indicator and version label remain on the left side of the footer bar; the three buttons (Discard Changes, Apply, Save & Apply) remain on the right side; internal spacing and control styling are unchanged. The footer background uses `HoloniightPalette.surface` to match the approved mockup.

**REQ-F-5.4 — Footer Height Unchanged**

The footer shall maintain its current height of 56 pixels.

**Acceptance Criterion**:
- `FooterBar { height: 56 }` or equivalent is set in the QML; visual verification confirms the footer height does not change after refactoring.

**REQ-F-5.5 — Footer No Frame Wrapping**

The footer shall NOT be wrapped in an `HnSurfaceFrame` or any custom border/corner-shaping component. It remains a flat, full-width bar.

**Acceptance Criterion**:
- Code review: no `HnSurfaceFrame { ... FooterBar ... }` wrapping exists; FooterBar is instantiated directly without a framing layer.

---

### F6: Background and Visual Continuity

**REQ-F-6.1 — Window Background Color**

The root window's background color shall be set to `HoloniightPalette.background`.

**Acceptance Criterion**:
- `Rectangle { color: HoloniightPalette.background }` is set on the root item or the ColumnLayout fills with this color; the visible gaps between the columns and edges show this background color.

**REQ-F-6.2 — No Native Window Frame**

The root `SettingsWindow.qml` shall NOT be wrapped in or adorned with an `HnSurfaceFrame`, custom border, or any other frame-drawing component that mimics a native window border. Only the three internal columns are framed.

**Acceptance Criterion**:
- Code review: no `HnSurfaceFrame { ... root ... }` or equivalent wrapping exists at the root level; the window's outer edge is a simple edge, not a drawn frame.

---

## Non-Functional Requirements

### NF1: Performance

**REQ-NF-1.1 — Layout Recalculation Efficiency**

Resizing the window (changing `QQuickView`'s width or height) shall trigger at most one layout pass for the ColumnLayout and one pass for the RowLayout, with no cascading re-layouts of page content.

**Acceptance Criterion**:
- Profiling via Qt's layout debug output (`QT_DEBUG_QML_BINDING=1` or layout inspector) shows a single pass through the root ColumnLayout and RowLayout when resizing the window; ContentStack does not trigger unnecessary reloads.

**REQ-NF-1.2 — Frame Rendering Overhead**

The four `HnSurfaceFrame` instances (nav, content, right panel, and implicit root background) shall render without perceptible jank (≤1 frame stall on a typical desktop GPU) when the window is first shown or resized.

**Acceptance Criterion**:
- Live testing: no visible frame drops, stutters, or delays when opening the settings window or resizing it; frame-rate monitoring (e.g., `QML_PROFILE=true`) shows no multi-frame stalls attributable to frame rendering.

---

### NF2: Consistency

**REQ-NF-2.1 — Semantic Role Consistency**

Both the nav and right panels shall use `HnSurfaceRole.Panel` (raised, elevated appearance). The content area shall use `HnSurfaceRole.Window` (primary content container). This semantic distinction shall not be altered.

**Acceptance Criterion**:
- Code inspection: nav panel's HnSurfaceFrame has `surfaceRole: HnSurfaceRole.Panel`; right panel's HnSurfaceFrame has `surfaceRole: HnSurfaceRole.Panel`; content area's HnSurfaceFrame has `surfaceRole: HnSurfaceRole.Window`.

---

## Constraints

### C1: Sizing Constraints

**REQ-C-1.1 — Minimum Window Width**

The `QQuickView` minimum width shall be set to ensure the layout can never collapse into overlapping columns. The minimum width shall be calculated as:

```
nav_min_width (220) + right_width (320) + outer_gaps (8 + 8) + inter_gaps (8 + 8) + content_min (672) = 1244 pixels
```

This value shall be set via `QQuickView::setMinimumWidth()` in `apps/settings/src/SettingsApplication.cpp`.

**Acceptance Criterion**:
- `view_->setMinimumWidth(1244)` (or equivalent) is called in SettingsApplication.cpp; attempting to resize the window width below 1244 pixels via window manager or programmatic resize is rejected by the OS/compositor (the resize request is denied, not clamped).

**REQ-C-1.2 — Minimum Window Height**

The `QQuickView` minimum height shall be set to 480 pixels. After the footer and approved 8px spacing are accounted for, content and navigation remain usable through their scrolling containers.

**Acceptance Criterion**:
- `view_->setMinimumHeight(480)` is called in SettingsApplication.cpp; attempting to resize the window height below 480 pixels is rejected by the OS/compositor.

**REQ-C-1.3 — Default Window Size Unchanged**

The default window size when the settings application first opens shall remain 1200×800 pixels (unchanged from current behavior).

**Acceptance Criterion**:
- `view_->resize(1200, 800)` is still called in SettingsApplication.cpp; the window opens at this size on first launch (assuming no saved geometry from prior sessions).

---

### C2: Architecture Constraints

**REQ-C-2.1 — No Page Content Changes**

No modifications shall be made to `AppearancePage.qml`, `BarPage.qml`, `PlaceholderPage.qml`, or any other page component during this cycle.

**Acceptance Criterion**:
- Git diff on the `apps/settings/qml/Pages/` directory shows zero changes (or only changes to imports/spacing if strictly necessary to avoid breaking the layout).

**REQ-C-2.2 — Preserve Nav Panel Navigation Behavior**

The `NavPanel.qml` component's nav item list, order, page-switching logic, and internal styling shall remain unchanged. Its navigation container shall scroll when the available height cannot display every item.

**Acceptance Criterion**:
- The page list and `onPageRequested` signal behavior remain unchanged; at the 480px minimum window height, users can scroll to and activate every navigation item.

**REQ-C-2.3 — No Footer Behavior Changes**

The footer's button behavior (Discard Changes, Apply, Save & Apply), enablement logic, and action handlers shall remain unchanged. Only its position in the layout changes.

**Acceptance Criterion**:
- `FooterBar.qml` retains its button handlers, enabled-state logic, and internal content alignment. Its background color is `HoloniightPalette.surface`.

**REQ-C-2.4 — Right Panel Scope Lock**

The new `PreviewPanel.qml` shall contain only a placeholder "Preview" label and no functional widgets, data bindings, or feature integrations (weather, now-playing, file browser, config mirrors, etc.).

**Acceptance Criterion**:
- Code review: PreviewPanel.qml contains only a Text element and structural layout items; no data properties are bound to BatteryService, WeatherService, AudioService, ConfigService, or any other application service; no Repeater, ListView, or dynamic model bindings exist.

---

### C3: QML Styling and Theming

**REQ-C-3.1 — No Hardcoded Colors**

All frame and panel colors shall be sourced from `HoloniightPalette` tokens (e.g., `HoloniightPalette.surface`, `HoloniightPalette.borderPassive`). No hardcoded hex values shall be used for fill or border colors.

**Acceptance Criterion**:
- Grep the `apps/settings/qml/SettingsWindow.qml` and `PreviewPanel.qml` files for hardcoded color values (e.g., `#ffffff`, `rgb(255, 255, 255)`); the search returns zero matches in fillColor/borderColor properties.

**REQ-C-3.2 — HnSurfaceFrame Import**

The `HnSurfaceFrame` component shall be imported from the shared `Holonight.Controls` module (imported as `import Holonight.Controls` and used as `HnSurfaceFrame { ... }`).

**Acceptance Criterion**:
- The import statement `import Holonight.Controls` appears in `SettingsWindow.qml` and `PreviewPanel.qml`; no custom or local frame component is defined or used.

---

## Verification & Testing

### V1: Manual Inspection Checklist

The following shall be verified manually in a live Hyprland session (not automated):

1. **Window opens with the requested 1200×800 default subject to its 1244px enforced minimum width, and layout is correct**: Nav panel on left (220px minimum, title-aware), content in center (fluid), right panel on right (320px), footer full-width below, with the approved 8px composition gaps. ✓
2. **Resize window larger**: all columns maintain their widths; content area expands. ✓
3. **Resize window smaller (toward 1244×480)**: columns shrink to minimum; resize to below minimum is rejected; every navigation page remains reachable by scrolling. ✓
4. **Corner topology visual check**: nav panel has left corners rounded + right corners chamfered; content has top-left + bottom-right rounded + other two chamfered; right panel has left corners chamfered + right corners rounded. ✓
5. **Colors match palette tokens**: nav and right panels are `surfaceRaised` with `borderPassive`; content area and footer are `surface`; gaps show `background`. ✓
6. **Footer content alignment preserved**: status + version on left, buttons on right, internal layout unchanged. ✓
7. **Page navigation**: switching between Appearance, Bar, and other pages loads correct page in content area; right panel stays visible on all pages. ✓
8. **Right panel has only "Preview" label**: no widgets, no dynamic content, no data bindings. ✓

### V2: Automated Testing

The following automated checks shall pass:

1. **QML Linting**: `task qml-lint` on `apps/settings/qml/` returns no new warnings related to the refactored files. ✓
2. **Build**: `task build` completes without errors or warnings. ✓
3. **QML Smoke Test**: If a QML unit test harness exists for settings, it shall instantiate the new layout without crashes or uninitialized-property errors. ✓

---

## Success Criteria (Summary)

The refactoring is complete and successful when:

1. **Layout structure**: `SettingsWindow.qml` uses a ColumnLayout with a RowLayout for the three columns and a separate FooterBar below.
2. **Framing complete**: Nav, content, and right panels are all wrapped in `HnSurfaceFrame` with correct roles and corner masks; footer is not framed.
3. **Spacing correct**: approved 8px outer-row, inter-column, row-to-footer, and side-panel inner gaps are visually verified.
4. **Right panel placeholder**: A new `PreviewPanel.qml` exists with only a "Preview" label, no functional content.
5. **Right panel always visible**: Navigating between all pages shows the panel on every page.
6. **Footer full-width**: Footer spans the full window width below all columns; internal content alignment is preserved.
7. **Minimum size enforced**: Window cannot be resized below 1244×480 (or close thereto).
8. **Build and lint pass**: No new build errors or linting warnings introduced.
9. **No scope creep**: Page content, navigation semantics, footer buttons, and preview content remain unchanged; navigation scrolling and the approved footer surface token are the only intentional internal adjustments.

---

## Document History

| Date | Author | Version | Notes |
|------|--------|---------|-------|
| 2026-07-30 | Claude Code | 1.0 | Initial specification; finalized after stakeholder grilling; ready for implementation planning. |
