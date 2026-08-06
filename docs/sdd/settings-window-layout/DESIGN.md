# Settings Window Layout Refactoring — Design

**Stage**: 2 (Architecture/Design) — SDD pipeline
**Source of truth**: `docs/sdd/settings-window-layout/SPEC.md` (Finalized v1.0). This document describes *how*
to implement that spec. Every `REQ-*` reference below is a citation, not a re-derivation; where this design
makes a judgment call to resolve spec ambiguity or a numeric inconsistency, it is called out explicitly under
**Key Decisions** and **Known Risks** rather than silently overridden.

---

## 1. Components

| File | Change | Responsibility after change |
| --- | --- | --- |
| `apps/settings/qml/SettingsWindow.qml` | Restructured | Root window item. Owns the `ColumnLayout` → `RowLayout` → three `HnSurfaceFrame`-wrapped columns → `FooterBar` tree. Sets root background, imports `Holonight.Controls`, instantiates and wires `NavPanel`, `ContentStack`, `PreviewPanel`, and `FooterBar` inside their frames. Still owns `currentPage`, the `HoloniightPalette.reload()` bootstrap, and the `fileService.onSaveFinished` connection — none of that changes. |
| `apps/settings/qml/NavPanel.qml` | Updated (see §4) | Left nav item list/order/activation behavior, unchanged. Its root becomes transparent so the wrapping frame is the sole paint source, and its navigation container becomes a `ListView` so every page remains reachable at the 480px minimum height. |
| `apps/settings/qml/ContentStack.qml` | **No change** | Page-switching `Loader`, unchanged. Root is already a transparent `Item`, so it needs no edit to coexist inside a wrapping `HnSurfaceFrame`. |
| `apps/settings/qml/FooterBar.qml` | Surface token updated | Internal `HnActionBar` layout and button behavior remain unchanged. Its background uses `HoloniightPalette.surface` to match the approved mockup. |
| `apps/settings/qml/PreviewPanel/PreviewPanel.qml` | **New file** | Placeholder right panel: a single `Text { text: "Preview" }`, no data bindings, no services. Root is a plain transparent `Item` (not `Rectangle`) so it never fights the wrapping frame's fill/corner shape, matching the pattern used for `ContentStack`. |
| `apps/settings/src/SettingsApplication.cpp` | Minimum-size values changed | Widens the existing `view_->setMinimumSize(...)` call from `QSize(1000, 700)` to `QSize(1244, 480)`. No reordering needed (see §4). |
| `apps/settings/CMakeLists.txt` | One line added | Registers the new `PreviewPanel.qml` file in `SETTINGS_QML_FILES` so it's compiled into the `HolonightSettings` QML module and resource-aliased. |

---

## 2. Layout tree diagram

### Before

```
Rectangle (root, color: HoloniightPalette.surface)
└── RowLayout (anchors.fill: parent, spacing: 0)
    ├── NavPanel (220px minimum, title-aware preferred width, Layout.fillHeight: true)
    └── ColumnLayout (Layout.fillWidth: true, Layout.fillHeight: true, spacing: 0)
        ├── ContentStack (Layout.fillWidth: true, Layout.fillHeight: true)
        └── FooterBar (Layout.fillWidth: true)
```

### After

```
Rectangle (root, color: HoloniightPalette.background)          [REQ-F-6.1]
└── ColumnLayout (mainColumn)                                   [REQ-F-1.1]
    anchors.fill: parent
    spacing: 8                                                   — 8px row↔footer gap [REQ-F-1.3]
    │
    ├── RowLayout (columnsRow)                                   [REQ-F-1.2]
    │   Layout.fillWidth: true
    │   Layout.fillHeight: true
    │   Layout.leftMargin: 8 | Layout.topMargin: 8 | Layout.rightMargin: 8
    │   spacing: 8                                                — 8px inter-column gaps [REQ-F-1.4]
    │   │
    │   ├── HnSurfaceFrame (navFrame)                             [REQ-F-2.1..2.6]
    │   │   surfaceRole: HnSurfaceRole.Panel
    │   │   chamferedCornersOverride: HnCornerMask.TopRight | HnCornerMask.BottomRight
    │   │   fillColor: HoloniightPalette.surfaceRaised
    │   │   borderColor: HoloniightPalette.borderPassive
    │   │   Layout.minimumWidth: 220 | Layout.preferredWidth: max(220, title + margins)
    │   │   Layout.fillWidth: false | Layout.fillHeight: true
    │   │   └── NavPanel { anchors.fill: parent; anchors.margins: 8; currentPage: root.currentPage; onPageRequested: ... }
    │   │
    │   ├── HnSurfaceFrame (contentFrame)                         [REQ-F-3.1..3.6]
    │   │   surfaceRole: HnSurfaceRole.Window
    │   │   (chamferedCornersOverride NOT set — stays Inherit)
    │   │   fillColor: HoloniightPalette.surface
    │   │   borderColor: HoloniightPalette.borderPassive
    │   │   Layout.fillWidth: true | Layout.fillHeight: true
    │   │   └── ContentStack { anchors.fill: parent; currentPage: root.currentPage }
    │   │
    │   └── HnSurfaceFrame (previewFrame)                         [REQ-F-4.1..4.6]
    │       surfaceRole: HnSurfaceRole.Panel
    │       chamferedCornersOverride: HnCornerMask.TopLeft | HnCornerMask.BottomLeft
    │       fillColor: HoloniightPalette.surfaceRaised
    │       borderColor: HoloniightPalette.borderPassive
    │       Layout.preferredWidth: 320 | Layout.fillWidth: false | Layout.fillHeight: true
    │       └── PreviewPanel { anchors.fill: parent; anchors.margins: 8 }
    │
    └── FooterBar                                                 [REQ-F-5.1..5.4]
        Layout.fillWidth: true
        (height: 56 unchanged; not wrapped in a frame [REQ-F-5.5])
```

Note that `Layout.*` attached properties move from the leaf components (`NavPanel`, `ContentStack`) onto the
wrapping `HnSurfaceFrame` instances, because `Layout.*` only has meaning on a `RowLayout`/`ColumnLayout`'s
*direct* children — see §4 for why this is the correct reading of REQ-F-2.5/2.6/3.5/3.6/4.5/4.6's "the nav
panel instance" / "the content area" phrasing.

---

## 3. Interfaces — exact `HnSurfaceFrame` bindings

### 3.1 Nav panel frame (`navFrame`)

```qml
HnSurfaceFrame {
    id: navFrame
    surfaceRole: HnSurfaceRole.Panel
    chamferedCornersOverride: HnCornerMask.TopRight | HnCornerMask.BottomRight  // 2 | 4 = 6
    fillColor: HoloniightPalette.surfaceRaised
    borderColor: HoloniightPalette.borderPassive
    Layout.minimumWidth: 220
    Layout.preferredWidth: Math.max(Layout.minimumWidth, navPanel.minimumContentWidth + 16)
    Layout.fillWidth: false
    Layout.fillHeight: true

    NavPanel {
        anchors.fill: parent
        anchors.margins: 8
        currentPage: root.currentPage
        onPageRequested: (key) => root.currentPage = key
    }
}
```

`TopRight | BottomRight` = `2 | 4` = `6`, matching REQ-F-2.1's stated evaluated value exactly. This chamfers
the two corners facing the content column and leaves the two corners facing the window edge rounded
(REQ-F-2.2).

### 3.2 Content frame (`contentFrame`)

```qml
HnSurfaceFrame {
    id: contentFrame
    surfaceRole: HnSurfaceRole.Window
    // chamferedCornersOverride intentionally NOT set — see REQ-F-3.1 / §4.3 below
    fillColor: HoloniightPalette.surface
    borderColor: HoloniightPalette.borderPassive
    Layout.fillWidth: true
    Layout.fillHeight: true

    ContentStack {
        anchors.fill: parent
        currentPage: root.currentPage
    }
}
```

`HnSurfaceRole.Window`'s own default kind is **Hybrid** (per the role table: top-right + bottom-left
chamfered, top-left + bottom-right rounded) at `chamferedCornersOverride: HnCornerMask.Inherit` (the property
default). Leaving the override unset produces exactly the topology REQ-F-3.2 requires — no override value is
needed or correct here.

### 3.3 Preview panel frame (`previewFrame`)

```qml
HnSurfaceFrame {
    id: previewFrame
    surfaceRole: HnSurfaceRole.Panel
    chamferedCornersOverride: HnCornerMask.TopLeft | HnCornerMask.BottomLeft  // 1 | 8 = 9
    fillColor: HoloniightPalette.surfaceRaised
    borderColor: HoloniightPalette.borderPassive
    Layout.preferredWidth: 320
    Layout.fillWidth: false
    Layout.fillHeight: true

    PreviewPanel {
        anchors.fill: parent
        anchors.margins: 8
    }
}
```

Bind the **named flags**, not a numeric literal — see the flagged discrepancy in §6 (the spec's stated
evaluated value of `3` does not match `TopLeft | BottomLeft` under the token table in
`theme-frames-usage.md`; the named-flag expression is what actually produces the REQ-F-4.2 visual (left
chamfered, right rounded) and is what must ship).

### 3.4 `PreviewPanel.qml` (new file)

```qml
import QtQuick
import Holonight.Core
import Holonight.Controls

Item {
    id: root

    Text {
        anchors.centerIn: parent
        text: qsTr("Preview")
        textFormat: Text.PlainText
        color: HoloniightPalette.textMuted
    }
}
```

`import Holonight.Controls` is required by REQ-C-3.2 even though this file doesn't directly reference
`HnSurfaceFrame` (the wrapping frame lives in `SettingsWindow.qml`) — REQ-C-3.2's acceptance criterion names
`PreviewPanel.qml` explicitly, so the import is added here regardless of whether it's strictly needed for
this file's own symbols. `HoloniightPalette.textMuted` (imported via `Holonight.Core`) satisfies REQ-C-3.1 —
no literal hex.

### 3.5 Root and outer layout (`SettingsWindow.qml`)

```qml
import QtQuick
import QtQuick.Layouts
import Holonight.Core
import Holonight.Controls

Rectangle {
    id: root
    width: 1200
    height: 800
    color: HoloniightPalette.background          // was HoloniightPalette.surface — REQ-F-6.1
    property string currentPage: "appearance"

    // ...existing Component.onCompleted / Connections{ target: fileService } unchanged...

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 8
            Layout.topMargin: 8
            Layout.rightMargin: 8
            spacing: 8

            HnSurfaceFrame { /* navFrame, see §3.1 */ }
            HnSurfaceFrame { /* contentFrame, see §3.2 */ }
            HnSurfaceFrame { /* previewFrame, see §3.3 */ }
        }

        FooterBar {
            Layout.fillWidth: true
        }
    }
}
```

`apps/settings/CMakeLists.txt` gains one entry:

```cmake
set(SETTINGS_QML_FILES
    qml/SettingsWindow.qml
    qml/NavPanel.qml
    qml/ContentStack.qml
    qml/PlaceholderPage.qml
    qml/FooterBar.qml
    qml/AppearancePage.qml
    qml/BarPage.qml
    qml/PreviewPanel/PreviewPanel.qml   # new
)
```

The existing `foreach` loop computes `QT_RESOURCE_ALIAS` relative to `qml/` via `cmake_path(RELATIVE_PATH ...)`,
so `qml/PreviewPanel/PreviewPanel.qml` gets alias `PreviewPanel/PreviewPanel.qml` automatically — no loop
changes needed. `qt_add_qml_module` registers every `QML_FILES` entry as a type in the single `HolonightSettings`
module namespace regardless of its subdirectory (the same mechanism `apps/shell/CMakeLists.txt` relies on for
`Topbar/TopBar.qml` etc.). `SettingsWindow.qml` imports `HolonightSettings` so `qmllint` can resolve the
subdirectory-registered `PreviewPanel` type. Runtime compilation succeeds without the self-import, but the
lint target reports `PreviewPanel` as unresolved, so the explicit import is retained for clean tooling.

`SettingsApplication.cpp`:

```cpp
view_->setMinimumSize(QSize(1244, 480));   // was QSize(1000, 700)
view_->resize(1200, 800);                  // unchanged — REQ-C-1.3
```

---

## 4. Key decisions with rationale

**Wrap externally, don't modify NavPanel/ContentStack internally to draw their own frame.** `HnSurfaceFrame`
is the single source of truth for the shape/geometry API (per `theme-frames-usage.md`'s design principle 1:
"select shape by semantic surface role, not a local radius literal"). Pushing frame-drawing into each leaf
component would duplicate that logic three times and couple unrelated components (`NavPanel`, `ContentStack`,
the new `PreviewPanel`) to frame-shape concerns that belong to the window-composition layer. Wrapping in
`SettingsWindow.qml` keeps `NavPanel`/`ContentStack` focused purely on their own content and matches how
REQ-F-2.1/F-3.1/F-4.1 are phrased ("wrapping the NavPanel instance" / "wrapping ContentStack" — an external
relationship, not an internal one).

**`Layout.*` properties live on the `HnSurfaceFrame`, not on `NavPanel`/`ContentStack`/`PreviewPanel`.**
Once a leaf component becomes a *child* of `HnSurfaceFrame` rather than a direct child of `RowLayout`, its own
`Layout.preferredWidth`/`Layout.fillWidth`/`Layout.fillHeight` bindings become inert (Qt Quick Layouts only
honors `Layout.*` attached properties on a layout's immediate children). REQ-F-2.5/2.6, F-3.5/3.6, and
F-4.5/4.6 phrase this as being set "on the nav panel instance" / "the content area" — read in design terms,
that means the wrapping `HnSurfaceFrame`, which is the RowLayout's actual direct child and functionally *is*
"the nav panel column" from the row's perspective. The leaf components inside each frame get `anchors.fill:
parent` instead, exactly as the "Standard card" example in `theme-frames-usage.md` does.

**Footer stays unwrapped and un-framed** (REQ-F-5.5): its `HnActionBar` content already reads correctly as a
flat full-width bar; adding a frame would introduce a fourth shape/color decision with no spec requirement
behind it and no adjacency seam to resolve (the footer doesn't sit next to another column the way nav/content/
preview do).

**Minimum size lives in C++ (`SettingsApplication.cpp`), not QML.** `QQuickView` is a top-level native window;
its minimum-size negotiation with the compositor/window-manager happens through `QWindow::setMinimumSize()`
(there's no QML-side API that reaches the top-level window's own resize constraints — QML can only set sizes
of *items* inside the scene, not the hosting `QQuickView`). This is unchanged from the current code, which
already does this correctly; the design widens the existing call's values, not its location.

**REQ-F-3.1's "no override" nuance is the one thing implementers are most likely to get wrong.** The nav and
preview panels both need an *explicit* `chamferedCornersOverride` because `HnSurfaceRole.Panel`'s own default
kind is **Chamfered** (all four corners chamfered, per the role table) — that default is wrong for a
docked side panel, so it must be overridden with a directional two-corner mask. The content area is the
opposite case: `HnSurfaceRole.Window`'s own default kind is already **Hybrid** — precisely the topology
REQ-F-3.2 asks for — so setting *any* `chamferedCornersOverride` on it, even one that happens to compute to
the same mask, is unnecessary and, if done carelessly (e.g. copy-pasting the nav/preview pattern), risks
diverging from the appearance-config-driven `Inherit` behavior described in `theme-frames-usage.md`'s
resolution-order steps 4–7 (an explicit override always wins over the user's global corner-style preference,
where `Inherit` would continue to respond to it). Do not "helpfully" add
`chamferedCornersOverride: HnCornerMask.TopRight | HnCornerMask.BottomLeft` to `contentFrame` — leave the
property absent entirely.

**Nav panel width uses a 220px floor and grows only for its application title.** A fixed ceiling can elide the
title after localization or theme-font changes. `NavPanel.minimumContentWidth` exposes the full title implicit
width, and the wrapping frame adds its two 8px inner margins. This remains a fixed-content sizing policy rather
than a proportional column: ordinary window growth continues to go exclusively to the content column. The
nominal minimum-window calculation therefore uses 220px and resolves to 1244px; unusually long localized titles
may require a wider user-selected window while remaining readable.

**`NavPanel.qml`'s root `color` changes from `HoloniightPalette.surfaceElevated` to `"transparent"`.** This is
the one necessary, unavoidable edit to `NavPanel.qml` beyond "just wrapping it," and it exists because
`NavPanel.qml`'s root is a `Rectangle` (not `Item`) that currently paints its own opaque, square-cornered fill.
Left as-is, that fill would render as a child *on top of* `navFrame`'s shaped background (children paint after
their parent), producing a squared-off panel with the wrong color token (`surfaceElevated` instead of the
spec-mandated `surfaceRaised`) that visually masks the frame's rounded/chamfered corners entirely — defeating
REQ-F-2.2 and REQ-F-2.3 outright. See §6 for why this is flagged despite REQ-C-2.2's "only the addition of the
HnSurfaceFrame wrapping" phrasing. `ContentStack.qml` and the new `PreviewPanel.qml` don't have this problem:
`ContentStack`'s root is already a transparent `Item`, and `PreviewPanel.qml` is written from scratch as a
transparent `Item` specifically to avoid repeating the mistake.

**Navigation uses a `ListView` rather than a fixed `ColumnLayout` + `Repeater`.** The approved 480px minimum
height cannot display all thirteen 40px navigation delegates simultaneously. A `ListView` preserves the
existing order, styling, and activation behavior at the default 800px height while making every page
reachable by scrolling at constrained heights.

**The 8px composition spacing and inner side-panel padding follow the approved mockup.** The root layout uses
an 8px row-to-footer gap; the row uses 8px left/top/right margins and 8px inter-column spacing. `NavPanel` and
`PreviewPanel` are inset 8px inside their frames. The footer itself remains flush with the window edges while
the existing `HnActionBar` keeps its 16px left/right content inset.

**Footer background uses `HoloniightPalette.surface`.** This is the only internal visual change to
`FooterBar.qml`; its action layout, button styling, handlers, and enablement logic remain unchanged.

---

## 5. Alternatives considered

- **Proportional/percentage-based column widths** — rejected. Navigation width is content-derived from a 220px
  floor, while preview width remains fixed; neither column grows merely because the window grows.
- **One shared `HnSurfaceFrame` wrapping the entire three-column row, with internal dividers drawn by hand**
  — rejected. A single frame instance resolves to exactly one `shapeKind`/`chamferedCorners` topology; it
  cannot simultaneously render three independent corner treatments (nav: right corners chamfered; content:
  hybrid; preview: left corners chamfered) inside one outer boundary. The spec requires three independently
  resolved topologies (REQ-F-2.2, F-3.2, F-4.2), which structurally requires three separate `HnSurfaceFrame`
  instances.
- **Wrapping `FooterBar` in its own `HnSurfaceFrame` for visual consistency with the three columns above it**
  — rejected. REQ-F-5.5 explicitly forbids this ("shall NOT be wrapped in an `HnSurfaceFrame` or any custom
  border/corner-shaping component"), and there was no stakeholder ask for it during Stage 1 grilling.
- **Giving `NavPanel`/`ContentStack` their own internal `HnSurfaceFrame` (frame-drawing pushed down into the
  leaf component instead of wrapped externally)** — rejected; see §4's rationale (couples unrelated
  components to shape concerns, duplicates the geometry API three times, and doesn't match how REQ-F-2.1/3.1/
  4.1 describe "wrapping the instance").

---

## 6. Known risks

1. **qmllint `Quick.layout-positioning` gotcha does not apply here, but is worth double-checking at review
   time.** The three `HnSurfaceFrame` instances become direct `RowLayout` children; per this project's own
   documented gotcha (CLAUDE.md), a direct Layout child that binds `width`/`height` directly to something
   (e.g. `height: parent.height`) trips this warning — the fix is to bind `implicitWidth`/`implicitHeight`
   instead. This design avoids the trap entirely by using only `Layout.preferredWidth` / `Layout.fillWidth` /
   `Layout.fillHeight` on the frames (no raw `width:`/`height:` bindings anywhere on them), which is the
   Layout-native sizing mechanism and not what the warning targets. Run `task qml-lint` after implementation
   to confirm — this note exists so a reviewer doesn't have to rediscover the mechanism from scratch if the
   lint does fire on some other line.

2. **`HnSurfaceFrame` does not clip children by default** (`theme-frames-usage.md`: "does not add content
   padding or clip children automatically... enable `clip` only when the product behavior requires it").
   This design does not set `clip: true` on any of the three frames, and audited every child that will render
   inside them: `ContentStack`'s `Loader` loads `AppearancePage.qml`/`BarPage.qml` (both `Flickable` roots
   with their own `clip: true`) or `PlaceholderPage.qml` (an `Item` with a centered `Text`, no background) —
   none paint an opaque background wider than their own bounds. `PreviewPanel.qml` is a single centered `Text`
   on a transparent `Item`. `NavPanel.qml`'s clipped `ListView` of `HnNavigationDelegate` items is the one case
   with any real fill area, but those are individual small delegate items inset inside the frame, so they
   don't threaten the frame's corner shape at the panel's outer boundary the way the panel's own root
   `Rectangle` did (see the `NavPanel.qml` fix in §4). **If any future page component (a non-goal for this
   cycle, but a live risk for whoever adds one later) fills its root with an opaque `Rectangle` sized to
   `anchors.fill: parent`, it will visually square off `contentFrame`'s hybrid corners** — flag this in review
   for any content-page PR, not just this one.

3. **Height budget at the enforced minimum (480px) requires scrolling.** With the actual layout tree, the
   three-column row receives approximately `480 − 8 (top margin) − 8 (row↔footer spacing) − 56 (footer) =
   408px`; the 8px inner frame margins leave about 392px for navigation. The thirteen-item navigation needs
   more height than that, so `NavPanel` uses a clipped `ListView`. Content pages retain their existing
   `Flickable` roots. Automated coverage verifies that the final navigation entry is reachable at this size.

4. **The 1244px minimum width preserves a 672px minimum content allocation at the nominal title width.** With
   the approved mockup geometry, the width budget is `220 (nav) + 320 (preview) + 16 (outer gaps) + 16
   (inter-column gaps) + 672 (content) = 1244`. A longer localized title expands the navigation preferred width
   rather than eliding; users can widen the window to retain the nominal center allocation.

5. **REQ-F-4.1's stated numeric mask value for the preview panel appears to be a documentation error, not an
   implementation instruction.** `HnCornerMask.TopLeft | HnCornerMask.BottomLeft` evaluates to `1 | 8 = 9`
   under the token table in `theme-frames-usage.md` (`TopLeft=1`, `TopRight=2`, `BottomRight=4`,
   `BottomLeft=8`), not the `3` stated in REQ-F-4.1's acceptance text (whose own parenthetical, "`0x0011` in
   binary: TopLeft=1, BottomLeft=8," doesn't itself add up to 3 either — `1+8=9`). This design binds the named
   flags (`HnCornerMask.TopLeft | HnCornerMask.BottomLeft`) rather than a literal numeric constant, which is
   both the only construct consistent with REQ-C-3.1 (no hardcoded magic values) and the only one that
   actually produces REQ-F-4.2's required visual (left corners chamfered, right corners rounded) — a literal
   `chamferedCornersOverride: 3` would chamfer `TopLeft | TopRight` instead, which is visually wrong. Flag this
   for whoever owns SPEC.md corrections; no design ambiguity remains for implementation, since the named-flag
   form is unambiguous regardless of which number the prose intended.

6. **`NavPanel.qml`'s one-line `color` edit technically exceeds REQ-C-2.2's "only the addition of the
   HnSurfaceFrame wrapping" acceptance phrasing.** As detailed in §4, this edit is not optional — without it,
   REQ-F-2.2/2.3 are unachievable because `NavPanel.qml`'s existing opaque `Rectangle` root paints over the
   wrapping frame's shape and color. Call this out explicitly during implementation review/PR description so
   it isn't mistaken for scope creep against REQ-C-2.2, and so a reviewer checking `git diff` against that
   acceptance criterion isn't surprised by a non-zero `NavPanel.qml` diff beyond the wrapping site.

7. **No existing automated test instantiates `SettingsWindow.qml` as a whole.** `tests/test_settings_app.cpp`
   (via `test_holonight_qml_harness`-style `QQmlComponent` tests) exercises `NavPanel.qml`, `FooterBar.qml`,
   `AppearancePage.qml`, and `BarPage.qml` individually, but nothing currently loads the composed
   `SettingsWindow.qml` tree. REQ-V-2.3 ("If a QML unit test harness exists for settings, it shall instantiate
   the new layout without crashes...") is therefore conditionally satisfied by omission — there's no existing
   harness entry point for the composed window, so there's nothing to update or break. Adding one is not
   required by any `REQ-*` in this cycle (no such requirement exists), but would be the natural way to catch a
   regression like risk #6 above in CI rather than only in the `task compositor-smoke-check` manual pass; noting
   this as a design-time observation only, not scoping it in.
