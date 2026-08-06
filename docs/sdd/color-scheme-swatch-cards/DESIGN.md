# Color Scheme Swatch Cards — Architecture Design

Stage 2 of SDD. Authoritative requirements: `docs/sdd/color-scheme-swatch-cards/SPEC.md` (33
REQ-F/REQ-NF/REQ-C items). This document is the literal blueprint for Stage 3 (task breakdown)
and Stage 4 (implementation) — file paths, property names, and signatures below are final unless
implementation surfaces a defect in this design.

---

## 1. Component Inventory

| File | Purpose | Satisfies |
|---|---|---|
| `apps/settings/src/theme_swatch_tokens.h` (new) | `ThemeSwatchTokens` QObject/QML_SINGLETON declaration | REQ-F-001, REQ-F-002, REQ-C-002 |
| `apps/settings/src/theme_swatch_tokens.cpp` (new) | Implementation: calls `Holonight::schemeKindForSchemeId` + `Holonight::tokensForScheme`, builds the 4-key `QVariantMap` | REQ-F-001–003, REQ-NF-005 |
| `apps/settings/qml/ColorSchemeSwatchCard.qml` (new) | `T.CheckDelegate`-based swatch card: surface fill, two gradient corners, checkmark badge, glow, accessible title | REQ-F-004–010, REQ-F-017–019, REQ-C-001, REQ-C-005, REQ-NF-003 |
| `apps/settings/qml/AppearancePage.qml` (modified) | Replace the "Color scheme" `RowLayout`+`Repeater`+`HnChoiceCard` block with `Flickable` + `Row` of `ColorSchemeSwatchCard`, edge-fade overlay | REQ-F-011–016, REQ-C-004 |
| `apps/settings/CMakeLists.txt` (modified) | Register `theme_swatch_tokens.h/.cpp` in the `holonight-settings` `qt_add_qml_module` `SOURCES` list; add `ColorSchemeSwatchCard.qml` to `SETTINGS_QML_FILES` | REQ-C-002, REQ-C-004, REQ-C-008 |
| `tests/test_settings_app.cpp` (modified, targeted) | Update `ThemeVariantsPreserveCatalogAndUpdateEditModel` for new card geometry only; interaction assertions unchanged | REQ-F-017–020, REQ-C-006 |

No changes to `holonight-qt`, `apps/shell/CMakeLists.txt`, or `Holonight.Controls` — confirmed
against REQ-C-003 and REQ-C-004.

---

## 2. Data Flow

### 2.1 Static/reactive chain

```
HolonightTheme.themeFamilies (existing QML singleton from holonight-qt, unchanged)
        │  Repeater { model: HolonightTheme.themeFamilies }
        ▼
ColorSchemeSwatchCard delegate instance (one per family)
        │  required property var modelData  →  familyId = modelData.id, title = modelData.name
        │  root.currentVariantId(modelData, editModel.themeMode)  [see 2.3]
        ▼
ThemeSwatchTokens.getTokensForScheme(currentVariantId)   (Q_INVOKABLE, called from a QML binding)
        │  Holonight::schemeKindForSchemeId(schemeId) → ThemeSchemeKind
        │  Holonight::tokensForScheme(kind) → Holonight::ColorTokens
        ▼
QVariantMap { surface, borderPassive, accent, secondaryAccent }
        │  bound directly into card properties (see §4)
        ▼
Rendered card: surface fill / borderPassive or accent border / two gradient corners / badge / glow
```

Because `getTokensForScheme` is called from a QML *property binding* (not an imperative
one-shot), it automatically re-evaluates whenever the binding's dependencies change — i.e.
whenever `currentVariantId` changes because `editModel.themeMode` toggled. This is what
satisfies REQ-F-013 (live mode reactivity) without any manual `Connections` block on the card
itself: the card's `schemeId` property is bound to an expression that already depends on
`editModel.themeMode`, so Dark Mode toggling invalidates it and QML re-invokes
`getTokensForScheme` with the new mode-matching variant id.

### 2.2 "which variant's tokens does a card preview" — resolved once, shared by paint + click

Both the **preview colors** (which variant's tokens paint the card) and the **click handler**
(which variant becomes `editModel.themeScheme`) need the same "family + current mode → variant
id" lookup used today in `AppearancePage.qml`'s `onClicked` (`modelData.variants.find(variant =>
variant.mode === editModel.themeMode)`). Promote this to a named helper on `AppearancePage.qml`'s
root (it already has `familyIdForScheme`, so this is consistent with existing style):

```qml
function variantIdForFamilyAndMode(family: var, mode: string): string {
    const target = family.variants.find(variant => variant.mode === mode)
    return target ? target.id : family.variants[0].id  // defensive: never empty
}
```

The card receives the *resolved schemeId* as a plain (non-required) property computed by the
delegate binding — not the raw family object — so `ColorSchemeSwatchCard.qml` stays decoupled
from the family/variant catalog shape and only needs to know "give me a scheme id, I'll paint
it." This keeps `ThemeSwatchTokens` calls entirely inside the card component (REQ-C-002: "QML …
contains a call like `themeTokens.getTokensForScheme(familyId)`").

### 2.3 Click sequence (matches existing `HnChoiceCard.onClicked` semantics, REQ-F-012)

1. User clicks/taps/space-selects an unchecked `ColorSchemeSwatchCard` for family F.
2. `T.CheckDelegate`'s built-in `onClicked` fires (inherited, not overridden away — see §5).
3. Card's `onClicked` handler (declared same as the current `HnChoiceCard` block) reads
   `editModel.themeMode`, calls `AppearancePage.variantIdForFamilyAndMode(root.familyData,
   editModel.themeMode)`, and sets `editModel.themeScheme = target`.
4. `editModel.themeScheme` write triggers `SettingsEditModel`'s existing dirty-tracking / config
   write-back path — **unmodified**, per REQ-F-016/Non-Goals ("No changes to theme
   persistence/config").
5. `ButtonGroup` (exclusive) automatically flips `checked` on the old and new selected cards —
   Qt's built-in behavior, same as today.
6. Each card's `checked`-dependent bindings (border color, glow `visible`, badge `visible`)
   re-evaluate on the same frame; no manual `Connections` needed (REQ-F-008, REQ-F-009).

### 2.4 Initial-state sequence

On `AppearancePage.qml` load, `Repeater` instantiates 5 cards. Each card's `checked` binding —
`root.familyIdForScheme(editModel.themeScheme) === familyId` — evaluates immediately using the
*already-loaded* `editModel.themeScheme`, exactly as today's `HnChoiceCard` binding does. No
special-cased "first paint" logic is needed; this is a plain declarative binding.

---

## 3. C++ Interface: `ThemeSwatchTokens`

### 3.1 Header — `apps/settings/src/theme_swatch_tokens.h`

```cpp
#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class ThemeSwatchTokens : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

 public:
  explicit ThemeSwatchTokens(QObject* parent = nullptr);

  // Returns exactly 4 keys: "surface", "borderPassive", "accent", "secondaryAccent".
  // schemeId is defensively resolved by Holonight::schemeKindForSchemeId, which already
  // falls back to ThemeSchemeKind::HoloNightDark for any id it cannot match to a known
  // theme variant (see theme_catalog.cpp: schemeKindForSchemeId -> themeVariantForSchemeId
  // -> nullptr -> HoloNightDark). No additional validation is required here.
  Q_INVOKABLE [[nodiscard]] QVariantMap getTokensForScheme(const QString& schemeId) const;
};
```

Notes on the header vs. the SPEC's sketch:
- SPEC's sketch omits `QML_ELEMENT`; both are required together for `QML_SINGLETON` to register
  under the module's default type name (`ThemeSwatchTokens`) — `FontListModel.h` in this same app
  uses the equivalent `QML_ELEMENT`-only pattern for a non-singleton type, confirming the app's
  qmltyperegistrar path is active; `QML_SINGLETON` is additive on top of `QML_ELEMENT`.
- `#include <QtQml/qqmlregistration.h>` (not `<QtQml/qqml.h>`) is the modern header providing
  `QML_ELEMENT`/`QML_SINGLETON` — `FontListModel.h` uses `<QtQml/qqml.h>` (older style); either
  works, but prefer `qqmlregistration.h` per current Qt guidance. Match `FontListModel.h`'s style
  (`<QtQml/qqml.h>`) instead if Stage 4 wants strict local-file consistency — functionally
  identical, implementer's call, not architecturally significant.

### 3.2 Implementation — `apps/settings/src/theme_swatch_tokens.cpp`

```cpp
#include "theme_swatch_tokens.h"

#include <holonight/palette.h>
#include <holonight/theme_catalog.h>

ThemeSwatchTokens::ThemeSwatchTokens(QObject* parent) : QObject(parent) {}

QVariantMap ThemeSwatchTokens::getTokensForScheme(const QString& schemeId) const {
  const Holonight::ThemeSchemeKind kind = Holonight::schemeKindForSchemeId(schemeId);
  const Holonight::ColorTokens tokens = Holonight::tokensForScheme(kind);

  return QVariantMap{
      {QStringLiteral("surface"), tokens.surface},
      {QStringLiteral("borderPassive"), tokens.borderPassive},
      {QStringLiteral("accent"), tokens.primary},
      {QStringLiteral("secondaryAccent"), tokens.accentBlue},
  };
}
```

`QColor` converts to `QVariant` automatically and remains a `color`-typed value on the QML side
(no `#RRGGBB` string round-trip needed) — QML bindings like `color: tokens.surface` work directly
against the `QVariantMap` value. This satisfies REQ-F-003/REQ-NF-005 (semantic tokens only, zero
hex literals) since every value flows from `Holonight::ColorTokens`.

### 3.3 QML registration mechanism (verified against this app's existing pattern)

`apps/settings/CMakeLists.txt`'s `qt_add_qml_module(holonight-settings URI HolonightSettings ...)`
already lists `FontListModel.h/.cpp` in its `SOURCES` block, and `FontListModel.h` self-registers
via `QML_ELEMENT` — Qt's qmltyperegistrar (invoked automatically by `qt_add_qml_module`) scans
`SOURCES` for `QML_ELEMENT`/`QML_SINGLETON` macros and generates the registration, no manual
`qmlRegisterSingletonType` call in `main.cpp`/`SettingsApplication.cpp` is used or needed. Add:

```cmake
qt_add_qml_module(holonight-settings
    URI HolonightSettings
    VERSION 1.0
    RESOURCE_PREFIX "/"
    NO_IMPORT_SCAN
    QML_FILES ${SETTINGS_QML_FILES}
    SOURCES
        src/SettingsEditModel.h
        src/SettingsEditModel.cpp
        src/ConfigFileService.h
        src/ConfigFileService.cpp
        src/ThemeConfigFile.h
        src/ThemeConfigFile.cpp
        src/FontListModel.h
        src/FontListModel.cpp
        src/ShellStatusService.h
        src/ShellStatusService.cpp
        src/theme_swatch_tokens.h      # new
        src/theme_swatch_tokens.cpp    # new
)
```

and link `holonight` (the installed package providing `<holonight/theme_catalog.h>` /
`<holonight/palette.h>`) alongside the existing `HolonightQt::Theme`/`HolonightQt::Config`
targets already on `target_link_libraries(holonight-settings PRIVATE ...)` — confirm which CMake
target actually provides the installed `/usr/include/holonight/` headers (it is almost certainly
already linked transitively via `HolonightQt::Theme`, since `ThemeConfigFile.cpp` already
`#include <holonight/theme_catalog.h>` today and the app builds; **verify at Stage 4** that no
new `target_link_libraries` entry is actually required — likely none is, since the include already
works for `ThemeConfigFile.cpp` under the current link set).

QML consumption: because it's a singleton with default type name `ThemeSwatchTokens`, any QML
file that does `import HolonightSettings` gets it automatically (same as `FontListModel`) —
`AppearancePage.qml` already has this import, and `ColorSchemeSwatchCard.qml` needs to add it too
(it currently has no imports beyond what a bare `T.CheckDelegate` component needs). Call site:

```qml
readonly property var tokens: ThemeSwatchTokens.getTokensForScheme(root.schemeId)
```

### 3.4 Test-harness registration parity

`tests/test_settings_app.cpp` manually calls `qmlRegisterType<FontListModel>("HolonightSettings",
1, 0, "FontListModel")` before loading `AppearancePage.qml` via `QQmlComponent` +
`QUrl::fromLocalFile(...)` — this bypasses the compiled module's own qmldir/plugin (the test loads
raw `.qml` source, not the packaged module), so every `QML_ELEMENT`/`QML_SINGLETON` type the page
depends on must be manually registered in the test file too. **`ThemeSwatchTokens` needs an
equivalent line added to `tests/test_settings_app.cpp`**:

```cpp
static const int swatch_tokens_registration =
    qmlRegisterSingletonType<ThemeSwatchTokens>("HolonightSettings", 1, 0, "ThemeSwatchTokens",
        [](QQmlEngine*, QJSEngine*) -> QObject* { return new ThemeSwatchTokens(); });
Q_UNUSED(swatch_tokens_registration);
```

placed the same way as the existing `FontListModel` registration, at the top of each
`SettingsAppearanceQmlTest` test body that loads `AppearancePage.qml` (currently 3 tests do:
`ThemeVariantsPreserveCatalogAndUpdateEditModel`, `DarkModeTogglePreservesFamilyAndUpdatesEditModel`,
plus `UsesFourFramedSectionsWithInlinePaddedRows` at minimum loads the whole page too — grep all
`QQmlComponent(&engine, ... AppearancePage.qml)` call sites in the test file and add the
registration to each, matching the existing `FontListModel` registration's placement exactly).
This is a **new requirement not explicit in the SPEC** but implied by REQ-C-008 ("no undefined Qt
meta-type registration") — flagged as a Known Risk in §8 since missing it produces a runtime QML
"ThemeSwatchTokens is not a type" error only at test time, not at `task build` time.

### 3.5 Caching decision

**No caching.** `getTokensForScheme` recomputes on every call. Rationale:
- `Holonight::tokensForScheme` constructs a `ColorTokens` value (a flat struct of `QColor`/`int`
  members, no I/O, no allocation beyond the struct itself) — this is a cheap value-type build,
  not a parse or disk read.
- Only 5 cards exist (REQ-F-016), each with one binding dependency (`schemeId`), so at most 5
  calls happen on load and at most 5 more on each Dark Mode toggle — nowhere near the 50 ms
  budget in REQ-NF-001.
- A `QMap<QString, QVariantMap>` cache adds invalidation-free-forever complexity (scheme tokens
  are static per build, so a cache would never go stale) but buys measurable performance only if
  profiling at Stage 4 shows otherwise. Per the SPEC's own "Caching Consideration" section: add
  only if profiling demonstrates a need. Not built preemptively.

---

## 4. QML Component Tree

### 4.1 `ColorSchemeSwatchCard.qml` — property list

```qml
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Templates as T
import QtQuick.Shapes
import QtQuick.Effects
import Holonight.Core
import HolonightSettings

T.CheckDelegate {
    id: root

    // --- Test/accessibility contract (REQ-F-017–019, REQ-NF-003) ---
    objectName: "themeFamilyCard"
    required property string familyId
    required property string title
    required property string schemeId   // resolved variant id to preview + select (see §2.2)

    // checked / enabled / width / height are inherited from T.CheckDelegate — not redeclared.

    // --- Derived token lookup (§2.1) ---
    readonly property var tokens: ThemeSwatchTokens.getTokensForScheme(root.schemeId)
    readonly property color surfaceColor: tokens.surface
    readonly property color borderPassiveColor: tokens.borderPassive
    readonly property color accentColor: tokens.accent
    readonly property color secondaryAccentColor: tokens.secondaryAccent

    implicitWidth: 128
    implicitHeight: 96
    hoverEnabled: true
    Accessible.name: root.title
    Accessible.description: ""
    Accessible.role: Accessible.CheckBox

    onClicked: {
        // populated by AppearancePage.qml's delegate wiring, see §4.3 — kept here only
        // as a placeholder signature reminder; actual body lives at the call site because
        // it needs editModel + the family's variants array, neither of which this
        // component should know about (REQ-C-004 decoupling).
    }

    background: cardBackground
    Item {
        id: cardBackground
        anchors.fill: parent

        // z-order (CLAUDE.md "MultiEffect z-order" gotcha: MultiEffect must be declared
        // BEFORE any sibling that must render above it):

        // 1. Glow layer — declared first so the outer Rectangle (2) and gradient corners (3,4)
        //    paint over it, keeping the halo visually *behind* the card body.
        MultiEffect {
            id: glow
            anchors.fill: outerRect
            source: outerRect
            visible: root.checked
            shadowEnabled: true
            shadowColor: root.accentColor
            shadowBlur: 0.5
            shadowOpacity: 0.22
            shadowScale: 1.02
            autoPaddingEnabled: true
        }

        // 2. Outer card body: surface fill + state-dependent border.
        Rectangle {
            id: outerRect
            anchors.fill: parent
            radius: 10
            color: root.surfaceColor
            border.width: 2
            border.color: root.checked ? root.accentColor : root.borderPassiveColor
            antialiasing: true

            // 3. Top-right accent gradient corner (REQ-F-005).
            Rectangle {
                id: topRightGradient
                width: 44
                height: 36
                radius: 8
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: 6
                anchors.rightMargin: 6
                antialiasing: true
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: root.accentColor }    // right edge (start)
                    GradientStop { position: 1.0; color: root.surfaceColor }  // left edge (end)
                }
                // NOTE: QML Gradient.Horizontal paints position 0.0 at the item's LEFT edge
                // by default; to get "right-to-left, accent at the right edge" per REQ-F-005,
                // apply `rotation: 180` (or mirror via `transform: Scale{ xScale:-1 }`) OR swap
                // the practical reading: verify with a live pixel sample at Stage 4 — see §8
                // Known Risk (gradient orientation direction is easy to get backwards and the
                // SPEC's AC explicitly pixel-samples both edges).
            }

            // 4. Bottom-left secondary-accent gradient corner (REQ-F-006).
            Rectangle {
                id: bottomLeftGradient
                width: 52
                height: 36
                radius: 8
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.bottomMargin: 6
                anchors.leftMargin: 6
                antialiasing: true
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: root.secondaryAccentColor }  // left edge
                    GradientStop { position: 1.0; color: root.surfaceColor }          // right edge
                }
            }

            // 5. Selection badge (REQ-F-007) — circle + hand-drawn checkmark, top-right,
            //    overlapping the card edge slightly.
            Item {
                id: badge
                width: 26
                height: 26
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: -6
                anchors.rightMargin: -6
                visible: root.checked

                Rectangle {
                    anchors.fill: parent
                    radius: width / 2
                    color: root.accentColor
                    border.width: 2
                    border.color: root.surfaceColor  // separates badge from card fill
                    antialiasing: true
                }

                Shape {
                    anchors.fill: parent
                    anchors.margins: 6
                    preferredRendererType: Shape.CurveRenderer
                    ShapePath {
                        strokeWidth: 2.5
                        strokeColor: HoloniightPalette.onPrimary
                        fillColor: "transparent"
                        capStyle: ShapePath.RoundCap
                        joinStyle: ShapePath.RoundJoin
                        startX: 1; startY: 7
                        PathLine { x: 5; y: 11 }
                        PathLine { x: 13; y: 1 }
                    }
                }
            }
        }
    }
}
```

Property/behavior notes:
- `checked`, `enabled`, `width`, `height` are **inherited from `T.CheckDelegate`**, not
  redeclared — REQ-F-018/019's AC only requires they be *readable* via
  `property("checked")`/`property("enabled")`/`width()`/`height()`, which works for any
  `QQuickItem`-derived property whether it's locally declared or inherited from the C++ base.
  `HnChoiceCard.qml` (the precedent this replaces) follows the same pattern — it never
  redeclares `checked`/`enabled` either.
- `title` is `required` (not optional with a default) because every card is always instantiated
  with `modelData.name` present — matches REQ-C-001's derivation-from-`CheckDelegate` intent and
  keeps the property contract explicit for the Loader/delegate-required-property gotcha
  documented in this repo's CLAUDE.md.
- No `text:` property is set/used — `T.CheckDelegate.text` exists but REQ-F-010 explicitly
  forbids rendering title text on the card face, so `contentItem` is not overridden with a
  `Label`; `Accessible.name: root.title` is the only place `title` surfaces.

### 4.2 Corner-gradient direction — resolved reading

Re-reading REQ-F-005's own AC precisely: *"pixel color sample at the **right edge** differs from
surface (≈ accent); pixel sample at the **left edge** ≈ surface."* With `Gradient.Horizontal`,
QML's stop `position: 0.0` maps to the item's left edge and `1.0` to the right edge (Qt's
documented behavior). To get accent-at-right / surface-at-left, the correct stop assignment is
actually:

```qml
GradientStop { position: 0.0; color: root.surfaceColor }   // left edge
GradientStop { position: 1.0; color: root.accentColor }    // right edge
```

This supersedes the inline comment-flagged uncertainty in §4.1's code sketch — Stage 4 must use
**this** stop assignment (surface at 0.0/left, accent at 1.0/right) for the top-right rectangle,
and correspondingly for the bottom-left rectangle per REQ-F-006 ("secondaryAccent, left edge ≈
accentBlue; right edge ≈ surface"):

```qml
GradientStop { position: 0.0; color: root.secondaryAccentColor }  // left edge
GradientStop { position: 1.0; color: root.surfaceColor }          // right edge
```

(This second one matches the original draft — only the top-right corner's stops needed
correcting.) **Verify both live** at Stage 4 with an actual pixel sample per the SPEC's AC
methodology; do not trust this write-up over a live screenshot if they disagree.

### 4.3 `AppearancePage.qml` — "Color scheme" section restructure

Replace lines 105–138 (the `control: Component { RowLayout { ButtonGroup... Repeater... } }`
block for `colorSchemeRow`) with:

```qml
control: Component {
    Flickable {
        id: swatchFlickable
        implicitWidth: root.inlineControlWidth
        implicitHeight: 96
        contentWidth: swatchRow.implicitWidth
        contentHeight: height
        orientation: Qt.Horizontal
        flickableDirection: Flickable.HorizontalFlick
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        ButtonGroup {
            id: themeFamilyGroup
        }

        Row {
            id: swatchRow
            spacing: 8

            Repeater {
                model: HolonightTheme.themeFamilies

                delegate: ColorSchemeSwatchCard {
                    id: themeFamilyDelegate

                    required property var modelData
                    required property int index

                    familyId: modelData.id
                    title: modelData.name
                    schemeId: root.variantIdForFamilyAndMode(modelData, editModel.themeMode)
                    checked: root.familyIdForScheme(editModel.themeScheme) === familyId
                    ButtonGroup.group: themeFamilyGroup

                    onClicked: {
                        const target = themeFamilyDelegate.modelData.variants.find(
                            variant => variant.mode === editModel.themeMode)
                        if (target)
                            editModel.themeScheme = target.id
                    }
                }
            }
        }

        // Edge fade hint (REQ-F-015) — see §5.3 for the exact technique.
        EdgeFadeOverlay {
            anchors.fill: parent
            flickable: swatchFlickable
        }
    }
}
```

`ColorSchemeSwatchCard` is imported implicitly (same-directory QML components auto-resolve, no
`import "."` needed for files in `apps/settings/qml/` per this app's existing convention — none
of the other same-folder components in `AppearancePage.qml`'s siblings use an explicit
same-folder import either). `EdgeFadeOverlay` is a second new small component — see §5.3 for
whether it's a separate file or inlined.

---

## 5. Key Design Decisions & Rationale

### 5.1 Card dimensions — finalized from SPEC's tentative range

- **Outer card**: 128×96 px (within SPEC's 100–150×80–120 range; picked to be evenly divisible
  and to leave clear room for a 26px badge overlapping the top-right corner without touching the
  top-left gradient corner's mirror position).
- **Corner radius**: 10 px (mid-range of SPEC's 8–12).
- **Gradient rectangles**: top-right 44×36, bottom-left 52×36 (within SPEC's 30–50 px "per side"
  range; bottom-left is slightly wider to visually balance the badge's negative space in the
  opposite corner — a presentation choice, not a functional requirement, adjustable freely at
  Stage 4 without a spec/design change).
- **Badge diameter**: 26 px (within SPEC's 24–32).
- **Glow blur**: `shadowBlur: 0.5` — `MultiEffect.shadowBlur` is a normalized 0–1 value (Qt's
  API, not a pixel radius), unlike the SPEC's pixel-radius framing ("~4–8 px"); 0.5 is the same
  value already used at `apps/shell/qml/Notifications/ToastItem.qml:117` for an equivalent
  "subtle halo" effect on this codebase's only other live `shadowEnabled: true` precedent, and is
  the value proven correct by that live component.

### 5.2 ButtonGroup / checked-state wiring — old pattern preserved verbatim

The old `HnChoiceCard` block's `ButtonGroup { id: themeFamilyGroup }` +
`ButtonGroup.group: themeFamilyGroup` + `checked: root.familyIdForScheme(editModel.themeScheme)
=== familyId` pattern is **copied unchanged** onto `ColorSchemeSwatchCard`. Nothing about
swapping the base type from a custom `HnChoiceCard` component (itself already a `T.CheckDelegate`
— see §6.2) to a new `T.CheckDelegate`-based component changes how `ButtonGroup` attaches or how
`checked` is driven; both are `CheckDelegate` instances from `ButtonGroup`'s point of view. This
is why REQ-C-001's AC ("removing the component from the ButtonGroup breaks exclusivity") holds
without new code — it's inherent to `T.CheckDelegate` + `ButtonGroup.exclusive: true`, not
something this design adds.

### 5.3 Edge-fade technique

Use a **two-sided `MultiEffect`-free gradient mask**, not `MultiEffect`'s `maskEnabled` (masking
via MultiEffect requires a `maskSource` image and adds render-pass cost for a purely cosmetic 2-D
gradient that a plain `Rectangle` with `Gradient` achieves for free). Concretely, a small
`EdgeFadeOverlay.qml` component (or an inline `Item` — a separate file is cleaner since it's used
identically on both edges):

```qml
// apps/settings/qml/EdgeFadeOverlay.qml
import QtQuick

Item {
    id: root
    required property Flickable flickable

    readonly property bool showLeft: flickable.contentX > 1
    readonly property bool showRight:
        flickable.contentX + flickable.width < flickable.contentWidth - 1

    Rectangle {
        width: 32
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        visible: root.showLeft
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: Qt.rgba(0, 0, 0, 0.35) }
            GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.0) }
        }
    }
    Rectangle {
        width: 32
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        visible: root.showRight
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: Qt.rgba(0, 0, 0, 0.0) }
            GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.35) }
        }
    }
}
```

`Qt.rgba(0,0,0,alpha)` is a **black translucency overlay**, not a hardcoded hex color token — it
composites over whatever card is underneath at the edge, which is architecturally distinct from
"a swatch card's own color" (REQ-NF-005 governs card *token* colors; this is a UI chrome
darkening effect, analogous to `HoloniightPalette.scrim`). To stay fully consistent with the
"no hardcoded hex/color literal" spirit even here, prefer binding the fade's base color to
`HoloniightPalette.scrim` (already alpha-baked per `palette.h`'s `scrim` token doc comment:
`#00000099`) scaled via `Qt.rgba` with the token's RGB channels — **or** simpler: use
`HoloniightPalette.scrim` directly as one gradient stop and a fully-transparent copy of it as the
other (`Qt.alpha(HoloniightPalette.scrim, 0)` — QML 6.9+ — or `Qt.rgba(HoloniightPalette.scrim.r,
HoloniightPalette.scrim.g, HoloniightPalette.scrim.b, 0)` for broad compatibility). Decide the
exact expression at Stage 4; the structural design (two edge `Rectangle`s with horizontal
`Gradient`, visibility driven by `contentX`/`contentWidth` comparisons) is the locked-in part.

`showLeft`/`showRight` use a **1 px epsilon**, not exact `=== 0` comparisons, because `Flickable`
content position is a `real` and can carry sub-pixel floating point noise after a flick
deceleration settles — an exact `0` comparison risks a fade sticking visible by a fraction of a
pixel. This directly satisfies REQ-F-015's AC wording ("`contentX === 0`... fades disappear")
functionally while being robust to float noise; note the epsilon in review if REQ-F-015's AC is
interpreted as requiring a bit-exact `0` check (unlikely to matter for a visual hint).

### 5.4 Click semantics — `T.CheckDelegate`'s own `onClicked`, not a `TapHandler`/`MouseArea` swap

`T.CheckDelegate` already provides pointer + keyboard (`Space`) click handling internally (this
is exactly what the existing test's `QTest::keyClick(&window, Qt::Key_Space)` after
`forceActiveFocus()` on line 573 of `tests/test_settings_app.cpp` currently exercises against
`HnChoiceCard`, itself a `T.CheckDelegate`). **Nothing in this design touches or replaces that
internal handling** — `onClicked:` is a normal signal handler on the delegate instance, the same
shape as the existing `HnChoiceCard.onClicked` block. The CLAUDE.md `TapHandler` vs `MouseArea`
click-semantics gotcha and the `WheelHandler` verification caveat do **not** apply here: this
design introduces no `TapHandler` and no custom wheel handling — `Flickable`'s own built-in wheel
support (present on `Flickable` since Qt 5.x, distinct from the flagged `WheelHandler` QML type)
drives scroll, and `T.CheckDelegate`'s inherited click path is left completely alone. Flagged
explicitly in §8 as a "verify nothing regressed" item, not a "new risk introduced" item.

---

## 6. Alternatives Considered and Rejected

### 6.1 Data source location: shared `holonight-qt` vs. app-local (Option C — CHOSEN)

- **Option A — add `tokensForSwatch()`-style helper directly to `holonight-qt`**: rejected.
  `holonight-qt` is a shared library consumed by both `holonight-shell` and (per its own
  versioned public headers) potentially other consumers; a QML-facing convenience wrapper whose
  only purpose is feeding 4 specific token names into one settings-page widget doesn't belong in
  a general-purpose theme library, and REQ-C-003 explicitly forbids modifying it for this feature.
- **Option B — expose `Holonight::ColorTokens` wholesale to QML from the shared library** (e.g. a
  generic `Q_INVOKABLE QVariantMap allTokensForScheme(schemeId)` living in `holonight-qt`):
  rejected as over-broad — `ColorTokens` has ~60 fields; shipping all of them to QML for a
  4-field consumer adds needless surface area and marshaling cost, and still requires *some*
  glue class somewhere to narrow it down for this card's specific visual mapping (REQ-F-003).
- **Option C — new local `apps/settings/src/ThemeSwatchTokens`, calling existing installed
  `holonight` headers directly (CHOSEN)**: single-use, co-located with the one page that needs
  it, precedented by `ThemeConfigFile.cpp`'s existing direct `#include <holonight/theme_catalog.h>`
  pattern in this very app. Zero shared-library changes; smallest possible surface area.

### 6.2 Component base: `CheckDelegate` vs. extending `HnChoiceCard` vs. from-scratch `Item`

- **Extend/reskin `HnChoiceCard`**: rejected. `HnChoiceCard` (from `Holonight.Controls`, shared
  module) is itself a `T.CheckDelegate` wrapping a text-first `ColumnLayout` `contentItem`
  (`Label` for title, `Label` for description — see `holonight-qt/qml/controls/HnChoiceCard.qml`).
  The swatch card's visual is not "text card with different colors" — it needs two absolutely-
  positioned gradient rectangles, a badge overlay, and a glow layer with a very specific
  z-order, none of which fit inside `HnChoiceCard`'s `contentItem`/`background` split without
  fighting its existing `ColumnLayout` contentItem and duplicate-purpose `background` Item. It's
  also a shared-module component used by other call sites (e.g. `accentChoiceCard` in this same
  page) — repurposing it for one visual would risk destabilizing those other consumers or forking
  it in place, both worse than a small new local component.
- **From-scratch `Item` + manual `MouseArea`/keyboard handling**: rejected. Would have to
  reimplement `ButtonGroup` integration, focus/keyboard-Space handling, hover state, and
  accessibility role plumbing that `T.CheckDelegate` provides for free — pure duplicated risk for
  no benefit, and directly contradicts REQ-C-001's explicit mandate.
- **`T.CheckDelegate` as root, custom `background`/no default `contentItem` use (CHOSEN)**: gets
  `ButtonGroup`, keyboard, hover, and accessibility-role scaffolding for free; the entire card
  visual lives in `background` (§4.1) since REQ-F-010 forbids visible text in `contentItem`
  anyway, so `contentItem` is simply left at `T.CheckDelegate`'s default (invisible/unused,
  since no `text:` is set and no custom `contentItem` is declared).

### 6.3 Scroll mechanism: `Flickable` vs. `ScrollView`/`ListView`+snapping vs. chevron buttons

- **Chevron/arrow buttons**: explicitly forbidden by the SPEC's Non-Goals and REQ-F-014's AC
  ("no `Flickable` import or compilation errors" implies pure `Flickable`, and REQ-F-015
  explicitly forbids "dedicated chevron buttons or arrow glyphs"). Not reconsidered here — this
  is a locked constraint, not a design choice.
- **`ListView` with `orientation: ListView.Horizontal` + `snapMode`**: considered and rejected.
  `ListView` adds delegate recycling/section machinery this use case doesn't need (exactly 5
  fixed-count items, REQ-F-016), and `ListView.snapMode` (`SnapToItem`) was tempting for a
  "cards align to viewport" feel, but the SPEC's scroll spec (§"Scroll Behavior Details") never
  asks for snap-to-card behavior — only free drag/wheel pan with edge fades — so `ListView`'s
  extra machinery buys nothing and the CLAUDE.md `ListView.section.delegate`/`pragma
  ComponentBehavior: Bound` gotcha becomes a needless landmine for zero behavioral gain. Plain
  `Flickable` + `Row` + `Repeater` (CHOSEN) is the minimal component that satisfies REQ-F-014
  literally ("a horizontal `QtQuick.Flickable`... containing the 5 swatch cards").
- **`ScrollView` (Controls wrapper around Flickable)**: rejected — `ScrollView` adds a visible
  scrollbar affordance by default; the SPEC calls for edge fades as the *only* scroll-availability
  hint (no scrollbar mentioned, and REQ-F-015's AC implies the fade is the sole indicator). A bare
  `Flickable` avoids fighting `ScrollView`'s default `ScrollBar` visibility policy.

---

## 7. Test Impact Plan

**Recommendation: deliberate, minimal update to `ThemeVariantsPreserveCatalogAndUpdateEditModel`
— not a rewrite.** Per REQ-F-020/REQ-C-006, this is an explicitly *allowed* deliberate update, not
a silent regression, and must ship with an inline comment.

### 7.1 What stays identical (no change needed)

Walking `tests/test_settings_app.cpp:511–579` line by line against this design:

- L512–513: `qmlRegisterType<FontListModel>(...)` — **unchanged**, still needed.
- **NEW LINE REQUIRED** (not in original): register `ThemeSwatchTokens` as a singleton the same
  way (§3.4) — required because `ColorSchemeSwatchCard.qml` now references it via `import
  HolonightSettings`, which the test's raw-file `QQmlComponent` load path does not auto-register.
- L515–531: model/engine/component setup, window show — **unchanged**.
- L533–534: `findVisualChildren(root_item, "themeFamilyCard")`, `ASSERT_EQ(cards.size(), 5)` —
  **unchanged**. `objectName: "themeFamilyCard"` is preserved verbatim on the new component root
  (§4.1), and `Repeater { model: HolonightTheme.themeFamilies }` still yields exactly 5 (REQ-F-016).
- L539–549: reading `familyId`/`title`/`enabled`/`checked` per card — **unchanged**. All 4
  properties exist with identical semantics (§4.1); `familyId` and `title` are `required
  property string` set from `modelData`, `enabled`/`checked` are inherited `T.CheckDelegate`
  properties, same as today.
- L551–556: family-id ordering assertion, selected/target card lookup — **unchanged**. Ordering
  comes from `HolonightTheme.themeFamilies`'s catalog order, untouched by this feature.
- L560: `ASSERT_EQ(model.themeMode(), "dark")` — **unchanged**, model-level, no QML dependency.
- L562–563: `target_card->mapToScene(QPointF(target_card->width()/2.0,
  target_card->height()/2.0))` — **unchanged mechanically**, but the *numeric* scene position
  changes because card width/height change from `HnChoiceCard`'s `Layout.preferredHeight: 36` (a
  `RowLayout`-filled, variable-width card) to the new fixed 128×96 (§5.1). The test code itself
  needs **no edit** — `width()`/`height()` are read live from the actual rendered item, not
  hardcoded — but the *click will only land correctly if the target card is scrolled into view*
  inside the new `Flickable` (see 7.2).
- L564–569: hover + click assertions (`hovered`, `checked` flips on target and old-selected card)
  — **unchanged**. `T.CheckDelegate` provides `hovered` inherently; behavior identical.
- L571–574: `forceActiveFocus()` + `Key_Space` on the previously-selected card selects
  `holonight-dark` — **unchanged**. Keyboard-Space-to-select is `T.CheckDelegate` built-in
  behavior, not something this design touches.
- L576–578: disabled-card click-is-a-no-op check — **unchanged**. `enabled` gating is
  `T.CheckDelegate` built-in.

### 7.2 What needs a deliberate, commented update

**One addition is required**: because the cards now live inside a `Flickable` with
`contentWidth` potentially exceeding `implicitWidth` (5 cards × 128px + 4×8px spacing = 672px,
likely exceeding `root.inlineControlWidth`'s typical ~180–420px range per
`AppearancePage.qml:16`), the `tokyonight` target card (3rd of 5) may not be fully visible/on
top of the Flickable's viewport at test time, and clicking at its `mapToScene` center could hit
a position that's been scrolled out of the visible clip region (clicks outside the Flickable's
visible bounds don't reach the delegate). Add, immediately before L562, with an inline comment
per REQ-F-020's required justification style:

```cpp
// Swatch cards now live in a horizontal Flickable (REQ-F-014); ensure the target card
// is scrolled into view before computing its scene position, since off-screen delegates
// inside a clipped Flickable cannot receive a synthesized click at their nominal geometry.
auto* swatch_flickable = findVisualChild(root_item, QStringLiteral("colorSchemeSwatchFlickable"));
ASSERT_NE(swatch_flickable, nullptr);
QMetaObject::invokeMethod(swatch_flickable, "resizeContent");  // or direct contentX set, see below
```

Simplest concrete form: give the `Flickable` in `AppearancePage.qml` (§4.3)
`objectName: "colorSchemeSwatchFlickable"`, and in the test set
`swatch_flickable->setProperty("contentX", <target_card_x_within_content>)` — computed as
`target_card->x()` relative to the Row, or more robustly, call a small QML-side helper. The exact
mechanics are an implementation-stage detail; the *design-level* requirement is: **the test must
guarantee the clicked card is within the Flickable's visible bounds before computing
`mapToScene`**, which it does not need to do today because the old layout was an unclipped
`RowLayout` where every card was always laid out (even if visually cramped, still hit-testable).

This is the only structural test change. It satisfies REQ-C-006 exactly as its own AC describes:
"deliberately updated with explicit comments justifying the change" — the interaction contract
(click-to-select-matching-mode-variant, keyboard Space, disabled-card no-op) is **fully
preserved**, only the pre-click scroll-into-view step is new, which is itself a **new assertion
of correct behavior** (REQ-F-020 explicitly permits this class of addition), not a weakening of
an existing one.

### 7.3 Other tests touching `AppearancePage.qml`

`UsesFourFramedSectionsWithInlinePaddedRows` (L335) and
`DarkModeTogglePreservesFamilyAndUpdatesEditModel` (L581) both load the full
`AppearancePage.qml` component and will need the same `ThemeSwatchTokens` registration line
(§3.4/§7.1) added, or they fail at `component.create()` with an unresolved-type QML error before
reaching any assertion. Grep every `QQmlComponent(&engine, ... AppearancePage.qml)` in the test
file at Stage 4 and add the registration uniformly — do not add it only to the one test this
design doc discusses in depth.

---

## 8. Known Risks / Open Implementation Questions

1. **Gradient stop direction (§4.2)** — the "which edge gets which stop position" mapping for
   `Gradient.Horizontal` is easy to get backwards; this design corrects one instance found during
   drafting (top-right corner) but Stage 4 must live-pixel-verify both corners per the SPEC's own
   AC methodology (screenshot + edge color sample), not trust static reasoning alone.

2. **Test-harness singleton registration gap (§3.4, §7.1, §7.3)** — every existing test that
   loads `AppearancePage.qml` raw (bypassing the compiled module) needs a new
   `qmlRegisterSingletonType<ThemeSwatchTokens>` line or it fails with an unresolved-type error at
   `component.create()`, not at `task build`. Easy to miss because `task build` and `task
   qml-lint` will both succeed while this is broken — only `ctest` surfaces it. Cross-reference
   this repo's CLAUDE.md testing note about `task configure-tests` staleness: since this doesn't
   add a new `tests/test_*.cpp` file, that particular trap doesn't apply, but the "silent
   omission only surfaces at runtime, not compile time" pattern is the same class of risk.

3. **Flickable click-target-visibility for tests (§7.2)** — the exact mechanism to scroll a card
   into view from C++ test code before computing `mapToScene` is sketched but not fully
   specified; Stage 4 should settle on either a `contentX` direct set or a small `Q_INVOKABLE`/
   QML function exposed for test use, whichever is less invasive.

4. **`T.CheckDelegate`'s default `contentItem`** — left un-set/default in §4.1 rather than
   explicitly nulled (`contentItem: null`); confirm at Stage 4 that `T.CheckDelegate`'s
   Basic-style default `contentItem` (typically a `Text`/`IconLabel`-ish item bound to `text:`,
   which this component never sets) doesn't render a stray empty-but-not-quite-invisible
   placeholder that steals layout space or paints an unwanted focus/ripple artifact from the
   `QtQuick.Controls.Basic` style. If it does, set `contentItem: Item {}` explicitly to neutralize
   it — cheap, low-risk mitigation, not applied preemptively since it may be unnecessary.

5. **`WheelHandler`/wheel-scroll verification** — per this repo's CLAUDE.md
   `WheelHandler`-vs-`MouseArea` gotcha, wheel input handling has a history of passing every
   automated check here (`qml-lint`, build, QML smoke test) while silently not working live on
   one topbar section. This design uses **`Flickable`'s own built-in wheel support**, not a
   custom `WheelHandler`, so the specific documented failure mode (a `WheelHandler` swapped in
   for a `MouseArea`) doesn't directly apply — but wheel-scroll-to-pan-horizontally on a
   `Flickable` whose primary `orientation` is horizontal is still worth a live manual check per
   this repo's general "verify live, not just via automated checks" posture for pointer-input
   behavior (REQ-NF-002's AC explicitly calls for live drag-scroll observation already).

6. **Edge-fade color source (§5.3)** — the exact `HoloniightPalette.scrim`-alpha-zero expression
   is left as a Stage 4 decision among a few equivalent forms; pick one and keep it consistent
   between the left and right `Rectangle`.

7. **`holonight` CMake link target (§3.3)** — this design assumes the existing
   `target_link_libraries(holonight-settings PRIVATE ... HolonightQt::Theme ...)` already
   transitively provides `<holonight/theme_catalog.h>`/`<holonight/palette.h>` (since
   `ThemeConfigFile.cpp` already includes `<holonight/theme_catalog.h>` and the app currently
   builds). Stage 4 should confirm no additional link line is needed rather than assume it.

8. **Card width vs. `root.inlineControlWidth`** — `HnSettingsRow`'s `control:` slot currently
   sizes inline controls to `root.inlineControlWidth` (a responsive 180–420px range,
   `AppearancePage.qml:16`), but 5 fixed 128px cards + spacing need ~672px of content width. The
   `Flickable`'s own `implicitWidth` should stay bound to `root.inlineControlWidth` (so the
   *row's allotted space* on the settings page doesn't change) while its `contentWidth` exceeds
   that — this is the expected/designed case that makes the Flickable's scroll behavior
   meaningful at all, not a bug, but worth calling out explicitly since it's a visible behavior
   change from the old `RowLayout` (which squeezed 5 `Layout.fillWidth: true` cards to fit,
   never scrolling).

---

## 9. Post-Implementation Amendments

Live mockup comparison during Stage 4 surfaced four deltas from this document's original design.
The code sketches in §3–§5 above are left as-drafted for historical context; treat the notes below
as authoritative for the shipped component. `SPEC.md` REQ-F-003, -005, -006, -007, -008, -009 carry
matching amendment notes.

1. **Accent token remap (§3.3, §5.x).** `ThemeSwatchTokens::getTokensForScheme()` originally mapped
   `accent` → `tokens.primary` and `secondaryAccent` → `tokens.accentBlue`. Both corners then read
   as visually similar (`primary` and `accentBlue` are close in most themes), which contradicted
   the intent of showing two *distinct* accents. Corrected to `accent` → `tokens.accentBlue` and
   `secondaryAccent` → `tokens.accentViolet` — the theme's actual named blue/violet accent pair
   (of the 3-accent cyan/blue/violet set each theme defines; cyan is unused here). The border
   (§4.1), top-right gradient (§4.2), and badge (§4.4) all consume `accent` (now blue); the
   bottom-left gradient consumes `secondaryAccent` (now violet).

2. **Glow effect removed entirely (§3.1 inventory, §4.3, §5's "Glow blur" rationale, §8.5).** The
   `MultiEffect { shadowEnabled: true }` drop-shadow glow on the selected card (REQ-F-009) was
   implemented as designed, then tuned once for visibility (`shadowOpacity` 0.22→0.4, `shadowBlur`
   0.5→0.6, `shadowScale` 1.02→1.04) after live review judged it too faint. At the increased
   intensity it still did not read well and was removed completely per explicit direction — the
   `MultiEffect` element and its now-unused `QtQuick.Effects` import are both deleted from
   `ColorSchemeSwatchCard.qml`. Selected-state affordance is now carried entirely by border color,
   the two gradient corners, and the badge.

3. **Badge inset inside the border, not overlapping it (§4.4, §8's 26px-badge note).** The design
   sketch positioned the badge to overlap/overflow the card's top-right edge. Mockup comparison
   showed it should sit fully inside the card frame instead; `anchors.rightMargin`/`topMargin` are
   bound to `root.cardPadding` (a positive inset, 4px) rather than a negative overflow offset.
   Badge diameter also shrank from the originally-sketched 26px to 22px to fit comfortably inside
   the smaller card (see next point).

4. **Card and corner-gradient resize (§4.1, §4.2).** The card shrank ~20% from 128×96 to
   102.4×76.8 (aspect ratio preserved) following mockup comparison; both corner-gradient rectangles
   had their height halved (36px → 18px, widths unchanged: 44px top-right, 52px bottom-left) to
   match the smaller card. This also exposed a bug in `AppearancePage.qml`'s `colorSchemeRow`
   control wrapper: its outer `Item.implicitHeight` was left at the stale pre-resize value (96)
   while the card's own `implicitHeight` had already moved to 76.8, so the `Flickable`'s
   `contentHeight: height` inherited the oversized wrapper and `Row`'s default top-alignment
   produced asymmetric top/bottom padding around the cards. Fixed by updating the wrapper's
   `implicitHeight` to match the card's (76.8).
