# Color Scheme Swatch Cards — Requirements Specification

> **Authoritative semantic-state amendment:** `ThemeSwatchTokens.getTokensForScheme()` retains its original keys
> and additionally exposes candidate `surfaceHover`, `surfacePressed`, `primary`, `onPrimary`,
> `selectionIndicator`, and `disabledOverlay` roles. Cards use `HnSurfaceFrame` Card/Pill geometry, candidate
> interaction colors and primary foreground contrast, candidate `selectionIndicator` for selection, and the
> active palette's `borderFocus`/`focusBorderWidth` for keyboard focus. `EdgeFadeOverlay` requires `fadeColor` and
> fades from the containing `surfaceElevated` plane; `scrim` is reserved for modal overlays.

## Feature Overview

Redesign the "Color scheme" selection control in the Settings window's Appearance page from a `RowLayout` of text-labeled `HnChoiceCard` delegates to a horizontally scrollable list of visually-iconic swatch cards. Each card renders a live preview of the theme family's current colors (surface, borders, accent tones) instead of text, providing users with immediate visual confirmation of theme appearance before commitment.

## Scope

- Replace the existing `Repeater` of `HnChoiceCard` components in `apps/settings/qml/AppearancePage.qml` with a new horizontal `Flickable` containing 5 new swatch card components.
- Implement a new C++ data class in `apps/settings/src/` to expose theme token colors for each scheme family.
- Implement a new QML component (derived from `QtQuick.Templates.CheckDelegate`) to render individual swatch cards.
- Preserve all existing selection behavior, config persistence, and test interaction contracts.
- Ensure swatch colors reflect live changes to Dark Mode toggle state.

## Non-Goals

The following are explicitly out of scope and must not be implemented:

- **No changes to the "Accent Color" control:** The separate accent-color picker elsewhere on the Appearance page remains unchanged.
- **No new variant selection UI:** The 10 individual theme variants (dark/light per family) continue to be selected implicitly via family selection + the existing Dark Mode global switch. No direct UI for picking among all 10 variants.
- **No changes to theme persistence/config:** All existing `ThemeConfigFile` and `SettingsEditModel` logic remains unmodified; only presentation layer changes.
- **No scroll chevron/arrow button:** Despite mockup depictions, no dedicated navigation buttons are added; drag and wheel scroll alone provide navigation.

---

# Functional Requirements

## F-1: Data Source Architecture

### REQ-F-001: Theme Tokens C++ Class (Ubiquitous)
The system shall provide a new C++ class (e.g., `ThemeSwatchTokens`) in `apps/settings/src/` that exposes, for any theme scheme ID (string), a `Q_INVOKABLE QVariantMap` method returning the keys: `surface` (color), `borderPassive` (color), `accent` (mapped from theme's `accentBlue` token), and `secondaryAccent` (mapped from theme's `accentViolet` token).

**AC:** A header file exists at `apps/settings/src/theme_swatch_tokens.h` or equivalent; the class has a `Q_INVOKABLE QVariantMap getTokensForScheme(const QString& schemeId)` method signature; the returned map contains exactly 4 keys; automated test verifies `map.contains("surface") && map.contains("borderPassive") && map.contains("accent") && map.contains("secondaryAccent")`.

### REQ-F-002: Token Source Dependencies (Ubiquitous)
The system shall call existing public functions from installed headers `<holonight/theme_catalog.h>` and `<holonight/palette.h>` (`Holonight::tokensForScheme()`, `Holonight::schemeKindForSchemeId()`) with no modifications to the `holonight-qt` repository.

**AC:** Grep `apps/settings/src/theme_swatch_tokens.*` finds includes of both `<holonight/theme_catalog.h>` and `<holonight/palette.h>`; build succeeds without errors; git diff shows no files modified outside the `holonight-shell/` directory tree.

### REQ-F-003: Token Mapping Rules (Ubiquitous)
The system shall map the following Holonight theme tokens to swatch-card visual slots:
- `accentBlue` token → accent-color corner gradient (top-right, right-to-left), outer border when selected, and selection badge
- `accentViolet` token → secondary-accent corner gradient (bottom-left, left-to-right)
- `surface` token → card background and gradient fade-to-color
- `borderPassive` token → unselected card border; accent-color replaces it when selected

**AC:** Code inspection verifies each of the 4 token-to-element mappings is implemented exactly once in `ThemeSwatchTokens::getTokensForScheme()` return value; live color-sample test confirms top-right corner/border/badge use `accentBlue` token, bottom-left corner uses `accentViolet`, outer fill is surface, outer border (when unselected) is borderPassive.

> **Amended (post-implementation):** originally `primary`/`accentBlue`. The theme palette exposes 3 named accents (`accentCyan`, `accentBlue`, `accentViolet`); the design was corrected to use two of the three explicitly-named accents (blue, violet) so the two gradient corners read as visually distinct, rather than one generic `primary` token and one accent that happened to share a name with the mapping key. `accentCyan` remains unused by this component.

---

## F-2: Visual Composition & Rendering

### REQ-F-004: Card Outer Geometry (Ubiquitous)
Each swatch card shall be a rounded rectangle with a controlled corner radius, filled with the theme's `surface` token color and bordered with the theme's `borderPassive` token color (in unselected state).

**AC:** QML component source file exists; live rendering on test compositor shows a single root rectangle with `radius > 0`, `color` property bound to `getTokensForScheme(...).surface`, and `border.color` bound to `getTokensForScheme(...).borderPassive`.

### REQ-F-005: Top-Right Accent Gradient Corner (Ubiquitous)
The card shall contain a small rounded-corner rectangle positioned in its top-right area with a linear gradient running right-to-left from the theme's blue accent color (`accentBlue` token) to the theme's `surface` token.

**AC:** Live rendering shows a visible rectangle in the top-right corner; pixel color sample at the right edge differs from the card's `surface` color (≈ accentBlue); pixel sample at the left edge of that rectangle ≈ surface color; `Gradient` element with `orientation: Gradient.Horizontal` and stops at 0.0 (accentBlue, right) and 1.0 (surface, left) is declared in QML.

> **Amended (post-implementation):** the original mockup used the generic `primary` token here. Live review showed the two corner gradients read as visually identical; the user clarified the intent was two *distinct* accents from the theme's 3-accent set (cyan/blue/violet) — blue for the top-right corner (and border/badge, REQ-F-008) and violet for the bottom-left corner (REQ-F-006). `ThemeSwatchTokens::getTokensForScheme()` now maps `"accent"` → `tokens.accentBlue` (was `tokens.primary`).

### REQ-F-006: Bottom-Left Secondary-Accent Gradient Corner (Ubiquitous)
The card shall contain a second small rounded-corner rectangle positioned in its bottom-left area with a linear gradient running left-to-right from the theme's violet accent color (`accentViolet` token) to the theme's `surface` token.

**AC:** Live rendering shows a visible rectangle in the bottom-left corner; pixel color sample at the left edge differs from the card's `surface` color (≈ accentViolet); pixel sample at the right edge ≈ surface color; `Gradient` element with `orientation: Gradient.Horizontal` and stops at 0.0 (accentViolet, left) and 1.0 (surface, right) is declared in QML.

> **Amended (post-implementation):** originally `accentBlue` (same token as REQ-F-005's corner, which is what made the two corners look identical). Now `accentViolet` — see the amendment note on REQ-F-005. The corner rectangle height was also reduced from 36px to 18px (width unchanged) as part of an overall 20% card-size reduction (128×96 → 102.4×76.8) driven by mockup comparison.

### REQ-F-007: Selection Badge (Ubiquitous)
When a card is selected, the system shall display a circular badge positioned in the card's top-right corner, inside the card's outer border, containing a checkmark symbol (drawn via `QtQuick.Shapes.ShapePath` or equivalent vector path, not an image asset).

**AC:** Live test: click a card to select it; screenshot captured within 100 ms of selection; a circular shape is visible in the top-right corner, fully inside the card's outer border; the circle contains a continuous tick/checkmark path; grep for `.png`/`.svg` image imports in the swatch-card component file returns no asset-based icon loads.

> **Amended (post-implementation):** originally specified as "overlapping the card's top-right area" (i.e. extending past the border edge). Mockup comparison showed the badge should sit fully inside the card frame instead; the badge's `anchors.rightMargin`/`anchors.topMargin` are now positive insets (`root.cardPadding`, 4px) rather than a negative overflow offset.

### REQ-F-008: Selected Card Border Styling (Ubiquitous)
When a card is in the selected state, the card's outer border color shall change from `borderPassive` to the theme's blue accent color (`accentBlue` token).

**AC:** Live test: capture screenshot of deselected card showing borderPassive border; click to select; capture screenshot; border color visually differs and matches the card's `accentBlue` token color; clicking a different card causes both old and new selection states to update borders correctly (old selection reverts to borderPassive, new selection gains accent border).

> **Amended (post-implementation):** originally `primary` token — see the amendment note on REQ-F-005; the border, top-right gradient, and badge all share the same `accent` (blue) token by design.

### REQ-F-009: Glow Effect on Selected Card — REMOVED (Ubiquitous)
~~When a card is selected, the system shall apply a subtle glow/shadow effect to the card using `QtQuick.Effects.MultiEffect { shadowEnabled: true }`.~~ **This requirement is removed.** Selected cards are distinguished by border color (REQ-F-008), gradient corners (REQ-F-005/006), and badge (REQ-F-007) only — no additional shadow/glow effect is applied.

**AC:** QML source inspection of `ColorSchemeSwatchCard.qml` confirms no `MultiEffect` element and no `QtQuick.Effects` import are present.

> **Amended (post-implementation):** a `MultiEffect` drop-shadow glow was implemented per the original mockup and tuned twice (increasing `shadowOpacity`/`shadowBlur`/`shadowScale` for visibility) during live review, but the user judged the effect did not read well at any tested intensity and requested it be removed entirely. The `QtQuick.Effects` import was removed as dead code alongside it.

### REQ-F-010: Card Title/Accessibility Label (Ubiquitous)
Each swatch card component shall expose a `title` property (e.g., the theme family name) used only for accessibility/`Accessible.name` declaration; the title text shall NOT be rendered visually as part of the card appearance.

**AC:** QML component root declaration includes `property string title: ""` or similar; root element binds `Accessible.name: title`; live screenshots and visual inspection show no text label overlaid on, above, below, or adjacent to the card shape itself; test code can read `card.property("title").toString()` and retrieve the family name.

---

## F-3: Selection & Interaction

### REQ-F-011: ButtonGroup Integration (Ubiquitous)
All 5 swatch cards shall participate in a single `QtQuick.Controls.ButtonGroup` with `exclusive: true`, preserving the existing one-card-selected-at-a-time behavior from the original `HnChoiceCard` layout.

**AC:** QML code inspection shows all cards defined within a `ButtonGroup { exclusive: true }` or referenced via `group: <ButtonGroup>` attached property; unit test: select card A via click simulation, verify `card_a.checked === true` and all others return `checked === false`; repeat for card B, verify counts match (exactly 1 selected at any time).

### REQ-F-012: Click-to-Select-Matching-Mode-Variant (Event-driven)
When the user clicks a swatch card for theme family F, the system shall select the theme variant within family F that matches the current `editModel.themeMode` (light or dark), exactly as the original `HnChoiceCard.onClicked` behavior did.

**AC:** Integration test: set `editModel.themeMode = "dark"`, click the holonight family card, verify `editModel.themeScheme.id` points to holonight's dark variant; externally set `editModel.themeMode = "light"`, click holonight card again, verify `editModel.themeScheme.id` now points to holonight's light variant; behavior repeated for all 5 families confirms mode-matching logic works consistently.

### REQ-F-013: Live Mode Reactivity (Event-driven)
When the user toggles the Dark Mode switch elsewhere on the Appearance page, all 5 swatch cards' preview colors shall update immediately to reflect the current `editModel.themeMode`.

**AC:** Live test setup: render all 5 cards with dark mode active; capture baseline screenshot with color samples at fixed relative positions on each card; toggle Dark Mode switch via UI; capture screenshot within 200 ms; compare color samples at same relative positions; RGB values differ between dark and light captures (surface/border/gradient colors all shift); no color property binding is latched or stale.

---

## F-4: Scroll Behavior & Layout

### REQ-F-014: Horizontal Flickable Container (Ubiquitous)
The system shall replace the existing `RowLayout` of cards with a horizontal `QtQuick.Flickable` (contentWidth > implicitWidth when needed) containing the 5 swatch cards, enabling drag-scroll and wheel-scroll navigation.

**AC:** QML code inspection shows a top-level `Flickable { orientation: Qt.Horizontal; ...}` wrapping the card repeater or row; dynamic test: programmatically set parent width to < 500px, verify Flickable's `contentWidth > width` returns true; drag-test: simulate left/right pointer drag on the cards, verify visual scroll response within 50 ms; static test: build succeeds with no `Flickable` import or compilation errors.

### REQ-F-015: Edge Fade Visual Hint (Ubiquitous)
The system shall render a subtle visual fade/gradient hint at the left and right edges of the Flickable when content is scrollable beyond the visible area, indicating more cards are available off-screen.

**AC:** Live screenshot with narrow viewport showing 3–4 cards visible: left edge displays a subtle fade (opacity ~20–40%, dark-on-dark or light-on-light gradient); right edge displays the same fade gradient; when Flickable is programmatically scrolled to fully show all cards (contentX === 0 and contentWidth <= parent.width), both fades disappear; no dedicated chevron buttons or arrow glyphs are rendered.

### REQ-F-016: Existing Repeater & Data Model Unchanged (Ubiquitous)
The system shall continue to drive card instantiation via `Repeater { model: HolonightTheme.themeFamilies }`, preserving the existing model-binding architecture; exactly 5 card instances are created, one per family.

**AC:** QML code inspection shows `Repeater { model: HolonightTheme.themeFamilies }` creating the swatch cards; automated test asserts `repeater.count === 5`; each `modelData` property from the repeater is accessible within the card delegate (e.g., bound to `familyId`); compile and runtime reveal no undefined model or missing properties.

---

## F-5: Backward Compatibility & Test Contract

### REQ-F-017: Preserved Card Identifiers (Ubiquitous)
Each swatch card component shall set `objectName: "themeFamilyCard"` so that existing automated tests can locate card instances via object name query.

**AC:** QML component root element sets `objectName: "themeFamilyCard"`; test code using `findChild<QQuickItem*>("themeFamilyCard")` locates all 5 card instances without modification; existing test `tests/test_settings_app.cpp` line containing this find operation compiles and succeeds.

### REQ-F-018: Exposed Card Properties for Test Inspection (Ubiquitous)
Each swatch card shall expose the following properties with their existing semantics:
- `familyId` (string): the theme family identifier (e.g., "holonight", "catppuccin")
- `checked` (bool): whether this card is currently selected
- `enabled` (bool): whether this card is clickable
- `title` (string): the theme family name or label for accessibility

**AC:** Component root declares all 4 properties via `property` or inherited from CheckDelegate; test code `card->property("familyId").toString()` returns a non-empty string; `card->property("checked").toBool()` returns true or false; `card->property("enabled").toBool()` returns true; `card->property("title").toString()` returns a non-empty family name string; all 4 read operations complete without "property not found" errors.

### REQ-F-019: Test Hit-Testing Compatibility (Ubiquitous)
Each swatch card shall have defined `width` and `height` properties such that existing scene-coordinate click tests (using `mapToScene()` and hit-testing) continue to function correctly.

**AC:** Test code `QPointF scenePos = card->mapToScene(QPointF(card->width() / 2, card->height() / 2))` returns a valid on-screen point (non-NaN, within compositor bounds); simulating a mouse click at that position successfully selects the card; follow-up test code `QCOMPARE(card->property("checked").toBool(), true)` passes.

### REQ-F-020: Deliberate Test Update Allowed (Constraint)
The existing test `tests/test_settings_app.cpp::ThemeVariantsPreserveCatalogAndUpdateEditModel` may require deliberate updates (e.g., assertions on card visual geometry/appearance) to accommodate the new swatch design; such updates are explicitly allowed and do not constitute a test regression, provided the interaction contract (click-to-select-matching-variant) remains intact.

**AC:** Test code inspection shows all `QCOMPARE`/`QVERIFY` assertions either pass unmodified or are deliberately updated with an inline comment (e.g., `// Updated for swatch card dimensions`) explaining the change; ctest runs `ThemeVariantsPreserveCatalogAndUpdateEditModel` and reports PASS (not FAIL); git log of the test file shows the update commit message justifies the changes.

---

# Non-Functional Requirements

## NF-1: Performance & Responsiveness

### REQ-NF-001: Card Rendering Performance (Ubiquitous)
The system shall render all 5 swatch cards with colors fetched from the theme-tokens C++ class in < 50 ms on the target development machine (Wayland Hyprland 0.55.2+), even on first appearance.

**AC:** Instrumented build with Qt performance logging or QML Profiler: measure time from "AppearancePage loaded" to "all 5 cards visible with colors applied"; log shows elapsed time < 50 ms; no `QCDebug` or warning messages indicate delayed token fetching or missing colors.

### REQ-NF-002: Flickable Scroll Smoothness (Ubiquitous)
Dragging or wheel-scrolling the Flickable shall maintain at least 60 fps frame rate; no jank or visually perceptible stutter during scroll or selection-badge animation transitions.

**AC:** Live test: simulate continuous drag-scroll left and right on the card list for 5 seconds; monitor reports sustained frame rate ≥ 58 fps; visual observation detects no frame drops, tearing, or stuttering; selection-badge appearance (if animated) maintains frame rate during simultaneous scroll.

---

## NF-2: Accessibility

### REQ-NF-003: Screen Reader Accessibility (Ubiquitous)
Each swatch card shall be identifiable and describable by screen readers via Qt's accessibility framework (e.g., `QAccessible` on C++ side, `Accessible.name`/`role` on QML side).

**AC:** QML component root binds `Accessible.name: title` (family name) and sets `Accessible.role: Accessible.Button` or `Accessible.CheckBox`; automated accessibility audit using `QTest::accessibleName()` or `QAccessible::queryAccessibleInterface()` confirms each card returns the expected family name string.

### REQ-NF-004: Keyboard Navigation (Ubiquitous)
All 5 swatch cards shall be selectable via keyboard (Tab navigation + Space/Enter to select), consistent with the original `HnChoiceCard` behavior in `ButtonGroup`.

**AC:** Live test: press Tab repeatedly in the Settings window; focus indicator moves to each visible swatch card in sequence; press Space on a focused card; test code verifies `card.checked === true` and `editModel.themeScheme` updated; behavior repeats for all 5 families.

---

## NF-3: Theme Compatibility

### REQ-NF-005: Semantic Theme Token Usage (Ubiquitous)
The system shall use only semantic theme tokens from the HoloNight design system (via `Holonight::tokensForScheme()` and `HolonightShell::HoloniightPalette` if QML access is needed); no hardcoded hex color values shall appear in QML or C++ source code for card colors.

**AC:** Grep `apps/settings/src/theme_swatch_tokens.*` for regex `#[0-9a-fA-F]{6}` returns no matches in color-assignment code (comments/docs excluded); grep `apps/settings/qml/ColorSchemeSwatchCard.qml` for same pattern returns no color literal assignments; all card surface/border/accent/gradient colors are derived from `getTokensForScheme()` return values or `HoloniightPalette` property bindings.

---

# Constraints

## C-1: Architecture Constraints

### REQ-C-001: QML Component Architecture (Ubiquitous)
The new swatch card QML component shall be derived from `QtQuick.Templates.CheckDelegate` (not a custom item from scratch), and shall participate in the existing `ButtonGroup` via its `checked` property and `group` attachment.

**AC:** Component root element is a `CheckDelegate { ... }` or wraps a direct `CheckDelegate` child wired to selection state; `CheckDelegate.group` is set to the parent `ButtonGroup` instance; removing the component from the ButtonGroup breaks exclusivity (verified by isolated test).

### REQ-C-002: C++ Class Integration (Ubiquitous)
The new C++ `ThemeSwatchTokens` class shall be registered with Qt's meta-object system (`Q_OBJECT`, `Q_INVOKABLE`) and exposed to QML via `QML_SINGLETON` or direct context property injection in `apps/settings/src/main.cpp`, allowing QML to call `themeTokens.getTokensForScheme(schemeId)` directly.

**AC:** QML code in `AppearancePage.qml` or `ColorSchemeSwatchCard.qml` contains a call like `themeTokens.getTokensForScheme(familyId)` with no C++→QML callback relay; `ThemeSwatchTokens` header declares `Q_OBJECT` and `Q_INVOKABLE QVariantMap getTokensForScheme(...)`; CMakeLists.txt includes `theme_swatch_tokens.cpp` in the `holonight_settings_app` target and links against `holonight` library.

### REQ-C-003: No holonight-qt Modifications (Constraint)
The feature shall make zero modifications to the sibling `holonight-qt` repository; all token-fetching logic shall use only existing public headers and functions.

**AC:** `git diff` between the feature branch and main shows no changes to files outside `holonight-shell/` directory; all `#include` statements in `ThemeSwatchTokens.cpp` reference installed public headers (`<holonight/...>`) from the pre-installed `holonight` package.

### REQ-C-004: QML Import Paths & Module Hierarchy (Ubiquitous)
The new swatch card component shall be placed in `apps/settings/qml/` and imported directly in `AppearancePage.qml` without requiring changes to the `HolonightShell` QML module registration or the shared `Holonight.Controls` module.

**AC:** Component source file exists at `apps/settings/qml/ColorSchemeSwatchCard.qml` (or similar); `AppearancePage.qml` imports it with a relative path (e.g., `import "."`); no modifications to `apps/shell/CMakeLists.txt` or shared module registration files; build succeeds with no import resolution errors.

### REQ-C-005: Glow Effect Framework — MOOT (Ubiquitous)
~~The glow effect on selected cards shall use `QtQuick.Effects.MultiEffect { shadowEnabled: true }` per the project's established convention (documented in CLAUDE.md); `Qt5Compat.GraphicalEffects.Glow` is forbidden.~~ **Moot: REQ-F-009 (the glow effect itself) was removed post-implementation; see its amendment note.** The constraint (MultiEffect over GraphicalEffects.Glow, should a glow ever be reintroduced) remains valid guidance but has nothing to check against in the current component.

**AC:** N/A — no glow effect exists in the current implementation to inspect.

---

## C-2: Test & Verification Constraints

### REQ-C-006: Existing Test Preservation (Constraint)
The automated test `tests/test_settings_app.cpp::ThemeVariantsPreserveCatalogAndUpdateEditModel` shall either pass unmodified after implementation or be deliberately updated with explicit comments justifying the change (e.g., "Card visual dimensions changed; updated expected geometry"); silent test breakage is not acceptable.

**AC:** Test harness runs `ctest -R ThemeVariantsPreserveCatalogAndUpdateEditModel` after feature completion; test result is either PASS (unmodified) or PASS (with git log showing a commit deliberately updating the test with inline comments); test result is never FAIL due to silent regression.

### REQ-C-007: QML Linting Compliance (Constraint)
The new swatch-card QML component shall pass `qml-lint` without errors or unresolved-import warnings.

**AC:** Run `task qml-lint` on the project after feature completion; output contains no error-level findings for `apps/settings/qml/ColorSchemeSwatchCard.qml`; warnings (if any) are reviewed and explicitly suppressed with `// qmlint disable <rule>` comments with justification.

### REQ-C-008: Build & CMake Integrity (Constraint)
Adding the new C++ class (`ThemeSwatchTokens`) and QML component shall not introduce CMake errors, undefined Qt meta-type registration, or linker failures.

**AC:** `task configure` completes without errors; `task build` completes without errors; executable runs without undefined-symbol errors; `task qmltypes-check` (if applicable) confirms no qmltypes registration gaps.

---

# Data Source Architecture Details

The `ThemeSwatchTokens` class shall implement the following interface:

## Header: `apps/settings/src/theme_swatch_tokens.h`

```cpp
class ThemeSwatchTokens : public QObject {
    Q_OBJECT
    QML_SINGLETON

public:
    explicit ThemeSwatchTokens(QObject* parent = nullptr);

    Q_INVOKABLE QVariantMap getTokensForScheme(const QString& schemeId) const;
};
```

## Return Value Structure

The `getTokensForScheme(schemeId)` method shall return a `QVariantMap` with exactly 4 keys:

| Key | Type | Source Token | Purpose |
|---|---|---|---|
| `"surface"` | `QColor` or `#RRGGBB` string | `tokens.surface` | Card background fill and gradient fade-to-color |
| `"borderPassive"` | `QColor` or `#RRGGBB` string | `tokens.borderPassive` | Unselected card border |
| `"accent"` | `QColor` or `#RRGGBB` string | `tokens.primary` | Top-right gradient start, selected border, badge |
| `"secondaryAccent"` | `QColor` or `#RRGGBB` string | `tokens.accentBlue` | Bottom-left gradient start |

## Token Retrieval Implementation

The class shall:

1. Accept a scheme ID string (e.g., "holonight_dark", "catppuccin_light")
2. Call `Holonight::schemeKindForSchemeId(schemeId)` to resolve the scheme kind
3. Call `Holonight::tokensForScheme(schemeKind)` to fetch the full token set
4. Extract the 4 tokens listed above from the returned token structure
5. Construct and return the QVariantMap with the 4 required keys

## QML Exposure

The class shall be exposed to QML as a singleton (via `QML_SINGLETON` or context property injection) so that QML components can call:

```qml
let colors = themeTokens.getTokensForScheme(familySchemeId)
let surfaceColor = colors.surface
let accentColor = colors.accent
```

## Caching Consideration

Tokens are fetched on-demand per `schemeId` call; optional caching within the class (e.g., `QMap<QString, QVariantMap>`) may be added at implementation stage if performance testing shows repeated calls to the same scheme ID are common.

---

# Visual Composition Specification

## Card Structure Diagram

```
┌─────────────────────────────────────┐
│ Outer: rounded rect                 │
│ • Fill: surface token               │
│ • Border: borderPassive (unselected) │
│ • Border: accent (selected)          │
│ • Corner radius: TBD                 │
│                                     │
│ ┌──────────┐            ●  ← Badge  │
│ │ Gradient │           ◑ ├─ Circle  │
│ │Top-Right │            ◐ ├─ Glow   │
│ │RTL       │           ○ └─ Check   │
│ │Accent→Srf│                        │
│ └──────────┘                        │
│                                     │
│     ┌──────────────┐                │
│     │ Gradient     │                │
│     │Bottom-Left   │                │
│     │LTR           │                │
│     │SecAcc→Srf    │                │
│     └──────────────┘                │
└─────────────────────────────────────┘
```

## Color Mapping

| Visual Element | Token | Condition | Details |
|---|---|---|---|
| Outer fill | `surface` | Always | Background of entire card |
| Outer border | `borderPassive` | Unselected | Default state |
| Outer border | `accent` | Selected | Switched on selection; may transition smoothly |
| Top-right gradient start | `accent` | Always | Right edge of rectangle |
| Top-right gradient end | `surface` | Always | Left edge of rectangle |
| Top-right gradient orientation | — | Always | Right-to-left (Gradient.Horizontal, stops 0→1 right-to-left) |
| Bottom-left gradient start | `secondaryAccent` | Always | Left edge of rectangle |
| Bottom-left gradient end | `surface` | Always | Right edge of rectangle |
| Bottom-left gradient orientation | — | Always | Left-to-right (Gradient.Horizontal, stops 0→1 left-to-right) |
| Selection badge circle fill | `accent` | Selected | Same as accent border |
| Selection checkmark stroke | TBD | Selected | Typically inverse of badge fill (white on dark accent, dark on light accent) or predefined contrast color |
| Glow halo | `accent` | Selected | Subtle shadow/blur around card perimeter using MultiEffect |

## Dimensions (to be finalized at Design stage)

The following dimensions are tentative and shall be confirmed through design mockup proportions:

- **Outer card:** ~100–150 px wide × ~80–120 px tall (aspect ratio ~1.2–1.5:1)
- **Corner radius:** ~8–12 px
- **Gradient rectangles:** ~30–50 px per side
- **Selection badge diameter:** ~24–32 px
- **Glow halo blur radius:** ~4–8 px (MultiEffect shadow)

---

# Scroll & Interaction Specification

## Layout Structure

```
AppearancePage.qml
  ├─ ColumnLayout (existing)
  │  ├─ [existing controls] (Color, Theme Mode, etc.)
  │  ├─ Label { text: "Color Scheme" }
  │  ├─ Flickable (horizontal)
  │  │  ├─ Row or inline card layout
  │  │  │  ├─ ColorSchemeSwatchCard (family 1: holonight)
  │  │  │  ├─ ColorSchemeSwatchCard (family 2: catppuccin)
  │  │  │  ├─ ColorSchemeSwatchCard (family 3: tokyonight)
  │  │  │  ├─ ColorSchemeSwatchCard (family 4: gruvbox)
  │  │  │  └─ ColorSchemeSwatchCard (family 5: cyber)
  │  │  └─ [edge fade overlay, if needed]
  │  └─ [existing controls below] (Accent Color, etc.)
  └─ [rest of page]
```

## Selection Behavior Details

### Initial State
On page load, the swatch card corresponding to the current `editModel.themeScheme` is selected (`checked: true`); all others are unchecked.

### Click Handler Logic
When the user clicks an unselected card:

1. **Retrieve current mode**: Read `editModel.themeMode` (string: "dark" or "light")
2. **Find matching variant**: Iterate through the clicked family's `variants` array to find the variant with `mode === editModel.themeMode`
3. **Update model**: Set `editModel.themeScheme` to the matching variant object
4. **ButtonGroup updates selection**: Qt's `ButtonGroup` automatically unchecks the previously-selected card and checks the newly-selected card

### Mode Toggle Reactivity
If the user toggles the Dark Mode switch:

1. **Mode property updates**: `editModel.themeMode` changes (dark ↔ light)
2. **Card preview colors update**: Each card re-fetches theme tokens via `getTokensForScheme()`, causing all surfaces/borders/gradients to repaint with the new mode's token values
3. **Selection persistence**: The currently-selected card remains selected; no automatic variant switch occurs (user may click a card again to select the light/dark variant matching the new mode, or the variant may already exist in the selected family in the new mode)

## Scroll Behavior Details

### No Scroll Required
If all 5 cards fit within the Flickable width (e.g., landscape display or large window):
- No edge fade is shown
- No scrollbar is visible
- All cards remain in view continuously

### Scroll Required
If the total content width exceeds the Flickable width (e.g., narrow portrait window):
- **Left edge fade**: When `contentX > 0` (not at the left extreme), render a subtle left-edge fade (~50–100 px wide, opacity ~20–40%) to indicate hidden cards to the left
- **Right edge fade**: When `contentX + width < contentWidth` (not at the right extreme), render a subtle right-edge fade at the right boundary to indicate hidden cards to the right
- **Fade disappears**: When scrolled to either extreme (`contentX === 0` for left, `contentX + width === contentWidth` for right), the corresponding fade disappears

### Drag & Wheel Interaction
- **Drag**: User can click and drag horizontally on the card row to pan left/right
- **Wheel**: User can scroll wheel (horizontal wheel or vertical wheel with modifier) to pan left/right
- **Momentum**: Optional smooth deceleration after drag release (Flickable default behavior)
- **No chevron buttons**: No dedicated left/right arrow buttons are provided; only drag/wheel and edge fades indicate more content

## Interaction Preservation

All existing interaction semantics are preserved:
- Exactly 1 card is selected at any time (ButtonGroup exclusivity)
- Clicking a card selects the mode-matching variant for that family (not a hardcoded variant)
- Selection affects `editModel.themeScheme`, which is then persisted via existing config logic
- No new config fields or persistence logic are introduced

---

# Backward Compatibility & Test Contract

## Existing Test: `ThemeVariantsPreserveCatalogAndUpdateEditModel`

### Current Behavior (before redesign)
The test:

1. Finds all `HnChoiceCard` instances via `findChild<QQuickItem*>("themeFamilyCard")`
2. Asserts `count === 5`
3. For each card, reads: `title`, `familyId`, `checked`, `enabled` properties
4. Simulates scene-coordinate clicks using `mapToScene()` + `QTest::mouseClick()` or similar
5. Asserts that clicking a card with `themeMode="dark"` selects that family's dark variant
6. Asserts that clicking a card with `themeMode="light"` selects that family's light variant

### Required Contract After Redesign

The same test **must** continue to:

1. **Locate cards**: Find all 5 instances via `objectName: "themeFamilyCard"` (unchanged)
2. **Read properties**: All 4 properties (`title`, `familyId`, `checked`, `enabled`) remain readable at the QML property level and return the same semantic values
3. **Simulate clicks**: Card `width` and `height` are non-zero and stable; `mapToScene()` calculations work correctly; click simulation selects the card
4. **Verify selection logic**: Clicking a card with `themeMode="dark"` selects the mode-matching dark variant; repeating with `themeMode="light"` selects the light variant

### Allowed Test Updates

Deliberate updates to the test are **permitted and encouraged** when:

- **Visual dimensions change**: If card width/height change from the original `HnChoiceCard` layout, click coordinate adjustments are necessary; update the test with a clear comment (e.g., `// Swatch card is 120px wide, centered in viewport; adjusted click offset`)
- **Layout changes**: If cards are now in a horizontal Flickable instead of a RowLayout, the test may need to ensure the target card is visible before clicking (e.g., `Flickable.contentX = 0` to show the first card)
- **Visual regression checks**: If implementation adds assertions on visual appearance (e.g., "selected card has glow"), new `QVERIFY` statements are acceptable (these are new tests, not regressions)

### Test Failure Guidance

- **Silent test breakage** (test runs but assertions fail with no explanation): **Not acceptable**. Update the test with inline comments explaining the new expectations.
- **Unmodified test fails**: Acceptable **only if** implementation issues are fixed to restore the contract (e.g., if `familyId` property is accidentally omitted, add it back to the component).
- **Unmodified test passes**: **Ideal**. No test changes required; full backward compatibility achieved.

## Property Contract Summary

Each swatch card must expose:

| Property | Type | Example Value | Test Access |
|---|---|---|---|
| `objectName` | string | `"themeFamilyCard"` | `findChild<QQuickItem*>("themeFamilyCard")` locates instance |
| `familyId` | string | `"holonight"`, `"catppuccin"`, etc. | `card->property("familyId").toString()` |
| `checked` | bool | `true` (selected) or `false` | `card->property("checked").toBool()` |
| `enabled` | bool | `true` (clickable) | `card->property("enabled").toBool()` |
| `title` | string | `"Holonight"`, `"Catppuccin"`, etc. | `card->property("title").toString()` |
| `width` | double | ~100–150 px | `card->width()` for `mapToScene()` calculations |
| `height` | double | ~80–120 px | `card->height()` for `mapToScene()` calculations |

---

# Requirement Summary Table

| ID | Category | Template | Short Description | Acceptance Criterion Type |
|---|---|---|---|---|
| REQ-F-001 | Functional | Ubiquitous | ThemeSwatchTokens C++ class with Q_INVOKABLE method | Code inspection + dynamic test |
| REQ-F-002 | Functional | Ubiquitous | Token dependencies from holonight public headers | Build success + git diff |
| REQ-F-003 | Functional | Ubiquitous | Token-to-visual-slot mapping rules | Code inspection + live color sample test |
| REQ-F-004 | Functional | Ubiquitous | Card outer geometry (rounded rect, surface fill, border) | Live rendering check |
| REQ-F-005 | Functional | Ubiquitous | Top-right accent gradient (RTL, accentBlue→surface) | Pixel color sample test |
| REQ-F-006 | Functional | Ubiquitous | Bottom-left secondary-accent gradient (LTR, accentViolet→surface) | Pixel color sample test |
| REQ-F-007 | Functional | Ubiquitous | Selection badge with vector checkmark, inset inside border | Screenshot + grep for asset imports |
| REQ-F-008 | Functional | Ubiquitous | Selected card border switches to accentBlue | Screenshot comparison + interaction test |
| REQ-F-009 | Functional | Ubiquitous | **REMOVED** — no glow/shadow effect on selected card | Code inspection (absence check) |
| REQ-F-010 | Functional | Ubiquitous | Card title property for accessibility (not rendered) | Property read test + screenshot |
| REQ-F-011 | Functional | Ubiquitous | ButtonGroup integration with exclusive selection | Unit test click simulation |
| REQ-F-012 | Functional | Event-driven | Click-to-select-matching-mode-variant behavior | Integration test mode flip + click |
| REQ-F-013 | Functional | Event-driven | Dark Mode toggle updates card colors live | Color sample comparison before/after |
| REQ-F-014 | Functional | Ubiquitous | Horizontal Flickable container | Code inspection + drag test |
| REQ-F-015 | Functional | Ubiquitous | Edge fade visual hint at scrollable edges | Live screenshot with narrow viewport |
| REQ-F-016 | Functional | Ubiquitous | Repeater driven by HolonightTheme.themeFamilies | Code inspection + count assertion |
| REQ-F-017 | Functional | Ubiquitous | Preserved objectName for test discovery | findChild test + existing test compile |
| REQ-F-018 | Functional | Ubiquitous | Exposed properties (familyId, checked, enabled, title) | Property read test for all 4 keys |
| REQ-F-019 | Functional | Ubiquitous | Test hit-testing compatibility (width/height defined) | mapToScene + click simulation |
| REQ-F-020 | Constraint | Ubiquitous | Deliberate test updates allowed; silent breakage not | Test result PASS or PASS (updated) |
| REQ-NF-001 | Non-Functional | Ubiquitous | Card rendering < 50 ms first-appearance | Performance profiling |
| REQ-NF-002 | Non-Functional | Ubiquitous | Scroll maintains ≥ 60 fps | Frame rate monitoring |
| REQ-NF-003 | Non-Functional | Ubiquitous | Screen reader accessibility (Accessible.name set) | Accessibility audit |
| REQ-NF-004 | Non-Functional | Ubiquitous | Keyboard navigation (Tab + Space/Enter) | Live keyboard interaction test |
| REQ-NF-005 | Non-Functional | Ubiquitous | Semantic theme tokens only (no hardcoded hex) | Grep + code inspection |
| REQ-C-001 | Constraint | Ubiquitous | Component derived from CheckDelegate | Code inspection |
| REQ-C-002 | Constraint | Ubiquitous | C++ class registered with Q_OBJECT/Q_INVOKABLE | QML invocation test |
| REQ-C-003 | Constraint | Ubiquitous | No holonight-qt repo modifications | git diff + include inspection |
| REQ-C-004 | Constraint | Ubiquitous | Component in apps/settings/qml/; no shared module changes | File location + build check |
| REQ-C-005 | Constraint | Ubiquitous | **MOOT** — glow removed, no effect to constrain | N/A |
| REQ-C-006 | Constraint | Ubiquitous | Existing test preserved or deliberately updated | ctest result + git log review |
| REQ-C-007 | Constraint | Ubiquitous | qml-lint compliance | qml-lint output review |
| REQ-C-008 | Constraint | Ubiquitous | Build & CMake integrity (no errors/linker failures) | task configure + task build |

---

# Sign-Off & Next Steps

This specification is **Stage 1 (Requirements)** of a Specification-Driven Development workflow. The following stages remain:

- **Stage 2 (Architecture)**: Design the C++ `ThemeSwatchTokens` class structure, QML component hierarchy, Flickable layout, and edge-fade implementation in detail.
- **Stage 3 (Implementation)**: Write C++ and QML code per the architecture design.
- **Stage 4 (Verification)**: Execute all acceptance criteria, run automated tests, perform live integration testing on Hyprland, and validate against this specification.

All decisions in this specification are **locked in** and represent the agreed-upon scope for the feature. Changes to scope must be explicitly documented and tracked in a supplementary change order.
