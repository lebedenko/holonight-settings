# SDD Tasks — color-scheme-swatch-cards

- [x] T-001: Implement ThemeSwatchTokens C++ header
  - REQs: REQ-F-001, REQ-F-002, REQ-C-002
  - Check: File exists at `apps/settings/src/theme_swatch_tokens.h` with `Q_OBJECT`, `QML_ELEMENT`, `QML_SINGLETON` macros and `Q_INVOKABLE QVariantMap getTokensForScheme(const QString&)` signature.

- [x] T-002: Implement ThemeSwatchTokens C++ implementation
  - REQs: REQ-F-001, REQ-F-002, REQ-F-003, REQ-NF-005
  - Check: File exists at `apps/settings/src/theme_swatch_tokens.cpp` calling `Holonight::schemeKindForSchemeId()` and `Holonight::tokensForScheme()`, returning a QVariantMap with exactly 4 keys: "surface", "borderPassive", "accent" (mapped from `accentBlue` token), "secondaryAccent" (mapped from `accentViolet` token). *(Amended post-implementation: originally primary/accentBlue — see SPEC.md REQ-F-003 amendment note.)*

- [x] T-003: Verify CMake link target provides holonight headers
  - REQs: REQ-F-002, REQ-C-008
  - Check: Confirm that existing `target_link_libraries(holonight-settings PRIVATE ...)` via `HolonightQt::Theme` already transitively provides `<holonight/theme_catalog.h>` and `<holonight/palette.h>` includes (verify by examining `ThemeConfigFile.cpp` which already uses these headers), or add explicit link if needed; build succeeds with no linker errors.

- [x] T-004: Register ThemeSwatchTokens in CMakeLists.txt
  - REQs: REQ-C-002, REQ-C-008
  - Check: `apps/settings/CMakeLists.txt` `qt_add_qml_module(holonight-settings ...)` SOURCES block includes both `theme_swatch_tokens.h` and `theme_swatch_tokens.cpp`; `task build` succeeds without CMake or linker errors.

- [x] T-005: Create ColorSchemeSwatchCard.qml component
  - REQs: REQ-F-004, REQ-F-017, REQ-F-018, REQ-F-019, REQ-NF-003, REQ-C-001
  - Check: File exists at `apps/settings/qml/ColorSchemeSwatchCard.qml` as a `T.CheckDelegate` root with `objectName: "themeFamilyCard"`, exposed properties `familyId`, `title`, `schemeId`, `checked`, `enabled`, `implicitWidth` (102.4), `implicitHeight` (76.8), and `Accessible.name` binding; `task qml-lint` finds no import or property errors in the component. *(Amended post-implementation: card shrunk ~20% from originally-planned 128×96 to 102.4×76.8 after mockup comparison — see DESIGN.md §9.)*

- [x] T-006: Render ColorSchemeSwatchCard outer geometry and surface fill
  - REQs: REQ-F-004, REQ-NF-005
  - Check: Component declares a `background` Item containing a `Rectangle` with `radius: 10`, `color: root.surfaceColor` (bound from tokens), `border.width: 2`, and `border.color` (state-dependent, see T-007); live screenshot shows a rounded rectangle filled with the selected theme's surface color.

- [x] T-007: Implement selected/unselected border styling
  - REQs: REQ-F-008
  - Check: `ColorSchemeSwatchCard`'s outer Rectangle binds `border.color: root.checked ? root.accentColor : root.borderPassiveColor`; live test: capture unselected card (borderPassive border), click to select (border changes to accent color), click another card (old selection reverts to borderPassive); color inspection confirms both state changes.

- [x] T-008: Implement top-right accent gradient corner
  - REQs: REQ-F-005, REQ-F-003
  - Check: Component declares a top-right positioned `Rectangle` (44×18, radius 8) with `Gradient { orientation: Gradient.Horizontal; GradientStop { position: 0.0; color: root.surfaceColor }; GradientStop { position: 1.0; color: root.accentColor } }` (verified equivalent after live pixel-sample test); live rendering shows gradient starts from right edge (accent/accentBlue) fading to left (surface). *(Amended post-implementation: rectangle height halved from originally-planned 36 to 18 to match the smaller card — see DESIGN.md §9.)*

- [x] T-009: Implement bottom-left secondary-accent gradient corner
  - REQs: REQ-F-006, REQ-F-003
  - Check: Component declares a bottom-left positioned `Rectangle` (52×18, radius 8) with `Gradient { orientation: Gradient.Horizontal; GradientStop { position: 0.0; color: root.secondaryAccentColor }; GradientStop { position: 1.0; color: root.surfaceColor } }`; live rendering shows gradient starts from left edge (secondaryAccent/accentViolet) fading to right (surface). *(Amended post-implementation: rectangle height halved 36→18; secondaryAccent now maps to accentViolet, not accentBlue — see SPEC.md REQ-F-006 amendment note.)*

- [x] T-010: Verify gradient stop directions live (manual visual test)
  - REQs: REQ-F-005, REQ-F-006
  - Check: Launch running holonight-shell, open settings Appearance page, capture a screenshot of a rendered swatch card, pixel-sample the top-right gradient rectangle at its right edge (color ≈ accentBlue), pixel-sample at its left edge (color ≈ surface); repeat for bottom-left rectangle left edge (≈ accentViolet) and right edge (≈ surface); all four samples match expected token colors.

- [x] T-011: Implement selection badge with checkmark
  - REQs: REQ-F-007, REQ-NF-005
  - Check: Component declares a `badge` Item (22px diameter, positioned top-right inset inside the card border by `root.cardPadding`) containing a `Rectangle` (circle via `radius: width / 2`, filled with `root.accentColor`, border in `root.surfaceColor`) and a `Shape` with `ShapePath` (checkmark path, stroke in `HoloniightPalette.onPrimary`); `visible: root.checked`; grep for `.png`/`.svg` imports in component returns no asset-based icon loads. *(Amended post-implementation: badge shrunk 26px→22px and repositioned from overlapping the card edge to inset inside the border — see SPEC.md REQ-F-007 amendment note.)*

- [x] T-012: ~~Implement glow effect on selected card~~ — REMOVED
  - REQs: REQ-F-009 (removed)
  - Check: `ColorSchemeSwatchCard.qml` contains no `MultiEffect` element and no `QtQuick.Effects` import. *(Amended post-implementation: a `MultiEffect` drop-shadow glow was implemented and tuned twice for visibility, but live review judged it did not read well at any tested intensity; removed entirely per explicit direction. See SPEC.md REQ-F-009 and DESIGN.md §9.)*

- [x] T-013: Implement token binding and live mode reactivity
  - REQs: REQ-F-013, REQ-F-003
  - Check: Component declares `readonly property var tokens: ThemeSwatchTokens.getTokensForScheme(root.schemeId)` and binds individual color properties; live test: render cards with dark mode, capture baseline color samples at fixed positions, toggle Dark Mode switch, capture within 200ms, compare RGB values at same positions (all differ between dark/light); no stale or cached bindings.

- [x] T-014: Create EdgeFadeOverlay.qml component
  - REQs: REQ-F-015, REQ-NF-005
  - Check: File exists at `apps/settings/qml/EdgeFadeOverlay.qml` as an Item with `required property Flickable flickable`, two `Rectangle` children (left 32px wide, right 32px wide), each with horizontal gradient (left fades dark-on-transparent left-to-right, right fades transparent-to-dark right-to-left), `visible` properties bound to `showLeft`/`showRight` (true when `flickable.contentX > 1` and `flickable.contentX + width < flickable.contentWidth - 1` respectively); no hardcoded hex colors in fade gradient construction.

- [x] T-015: Modify AppearancePage.qml to replace RowLayout with Flickable
  - REQs: REQ-F-011, REQ-F-012, REQ-F-014, REQ-F-016
  - Check: `apps/settings/qml/AppearancePage.qml` lines ~105–138 replaced with a Flickable block containing `ButtonGroup { id: themeFamilyGroup }`, a Row with spacing:8 wrapping a Repeater, each delegate is a ColorSchemeSwatchCard with bindings for `familyId`, `title`, `schemeId`, `checked`, `group`, and `onClicked` handler; Repeater instantiates exactly 5 cards (one per HolonightTheme.themeFamilies); no import errors.

- [x] T-016: Add variantIdForFamilyAndMode helper to AppearancePage
  - REQs: REQ-F-012, REQ-F-013
  - Check: `AppearancePage.qml` root declares a function `function variantIdForFamilyAndMode(family: var, mode: string): string` that finds and returns the variant id matching the given mode, with defensive fallback to first variant; this function is used to compute each swatch card's `schemeId` binding and in the card's `onClicked` handler.

- [x] T-017: Implement click-to-select behavior on swatch cards
  - REQs: REQ-F-012, REQ-F-011
  - Check: ColorSchemeSwatchCard delegate's `onClicked` handler reads `editModel.themeMode`, calls `variantIdForFamilyAndMode(modelData, mode)` to find the matching variant, sets `editModel.themeScheme` to the target variant; live test: set `editModel.themeMode = "dark"`, click holonight card, verify `editModel.themeScheme.id` matches holonight-dark; toggle mode to "light", click holonight again, verify schemeId now matches holonight-light.

- [x] T-018: Add EdgeFadeOverlay to Flickable
  - REQs: REQ-F-015
  - Check: Flickable block in AppearancePage.qml contains an EdgeFadeOverlay instance with `anchors.fill: parent` and `flickable: swatchFlickable`; live screenshot with narrow viewport showing 3–4 cards: left edge shows subtle fade when `contentX > 1`, right edge shows fade when content is scrollable; fades disappear when fully scrolled to either extreme.

- [x] T-019: Register ThemeSwatchTokens in test suite
  - REQs: REQ-C-002, REQ-C-008
  - Check: `tests/test_settings_app.cpp` contains a `qmlRegisterSingletonType<ThemeSwatchTokens>(...)` registration line identical in pattern to the existing `FontListModel` registration; registration is placed in every test function that loads AppearancePage.qml (minimum: ThemeVariantsPreserveCatalogAndUpdateEditModel, DarkModeTogglePreservesFamilyAndUpdatesEditModel, UsesFourFramedSectionsWithInlinePaddedRows); grep confirms all three test functions have the line.

- [x] T-020: Update ThemeVariantsPreserveCatalogAndUpdateEditModel test
  - REQs: REQ-F-017, REQ-F-018, REQ-F-019, REQ-F-020, REQ-C-006
  - Check: Test still locates all 5 cards via `findChild("themeFamilyCard")`, reads all 4 properties (familyId, title, checked, enabled) without errors, and verifies mode-matching selection logic; test includes a new code block (before click at scene coords) with an inline comment explaining that target card must be scrolled into view within the Flickable before `mapToScene` is computed, OR a helper QML function is added to ColorSchemeSwatchCard/AppearancePage to handle scroll-into-view; ctest runs the test and reports PASS.

- [x] T-021: Verify T.CheckDelegate default contentItem doesn't render artifacts
  - REQs: REQ-F-010, REQ-C-001
  - Check: Live test: render a ColorSchemeSwatchCard (checked and unchecked states), examine card visually for any stray text placeholder, empty-but-visible Label, or control artifact from CheckDelegate's default contentItem; if any unwanted element appears, set `contentItem: Item {}` explicitly in the component and re-verify; screenshot confirms no artifacts.

- [x] T-022: Verify ButtonGroup exclusivity and keyboard navigation
  - REQs: REQ-F-011, REQ-NF-004
  - Check: Live test: render Appearance page, Tab to first swatch card (focus indicator visible), press Space to select it, verify `card.checked === true` and all others are `checked === false`; Tab to second card, press Space, verify old selection unchecks and new one checks; repeat for all 5 families; behavior matches original HnChoiceCard ButtonGroup semantics.

- [x] T-023: Verify Flickable scroll and drag interaction
  - REQs: REQ-F-014, REQ-NF-002
  - Check: Live test: narrow settings window to ~200px width (forcing Flickable scroll), simulate left/right drag on cards, visual scroll response observed within 50ms, no jank or stutter; wheel-scroll horizontally on the card row, cards pan smoothly; monitor frame rate during drag/wheel (sustained ≥58 fps); existing scroll behavior identical to prior test on Hyprland 0.55.2+ compositor.

- [x] T-024: Verify live Dark Mode toggle reactivity
  - REQs: REQ-F-013, REQ-NF-001
  - Check: Live test: Appearance page open, all 5 cards rendered (dark mode), capture color samples at fixed card center positions, toggle Dark Mode switch, capture samples within 200ms, compare RGB values (all differ between captures indicating token refresh occurred); verify no color binding is latched or stale; measurement from "page load" to "all 5 cards visible with colors applied" is <50ms on development machine.

- [x] T-025: Verify card rendering performance
  - REQs: REQ-NF-001
  - Check: Instrumented build or profiler: measure time from AppearancePage component instantiation to all 5 swatch cards visible with colors fully applied; log shows elapsed time <50ms; no QCDebug warnings indicate delayed token fetching or missing colors.

- [x] T-026: Run qml-lint
  - REQs: REQ-C-007
  - Check: `task qml-lint` completes with no error-level findings for `apps/settings/qml/ColorSchemeSwatchCard.qml` or `apps/settings/qml/EdgeFadeOverlay.qml`; any warnings are reviewed and suppressed with inline `// qmlint disable <rule>` comments with justification; output shows clean lint status for Settings app.

- [x] T-027: Run task build
  - REQs: REQ-C-008
  - Check: `task configure` completes without errors; `task build` completes without errors or linker failures; no undefined Qt meta-type registration warnings; executable runs without symbol errors; task qmltypes-check passes if applicable (verify qmltypes file is non-empty and contains proper module definition, not just `Module {}`).

- [x] T-028: Run ctest and verify all tests pass
  - REQs: REQ-C-006, REQ-C-008
  - Check: `task test` (or `ctest` inside build/) runs all test suites without errors; ThemeVariantsPreserveCatalogAndUpdateEditModel test reports PASS; DarkModeTogglePreservesFamilyAndUpdatesEditModel test reports PASS; UsesFourFramedSectionsWithInlinePaddedRows test reports PASS; all three tests have ThemeSwatchTokens registration and any necessary scroll-into-view logic; no test failures or regressions.

- [x] T-029: Verify no holonight-qt repository modifications
  - REQs: REQ-C-003
  - Check: `git diff` between feature branch and main shows no files modified outside the `holonight-shell/` directory tree; all `#include` statements in `apps/settings/src/theme_swatch_tokens.cpp` reference only installed public headers from `<holonight/...>`; git log and file inspection confirm zero changes to sibling holonight-qt repo.

- [x] T-030: Final integration and accessibility verification
  - REQs: REQ-NF-003, REQ-NF-004, REQ-F-010
  - Check: Live test: Appearance page renders without crashes; swatch cards are reachable via Tab keyboard navigation; screen reader (or accessibility audit tool) reads each card's `Accessible.name` (family name) correctly; clicking/Space-selecting a card triggers the expected theme change and config persistence; no accessibility-related QML warnings in logs.

- [x] T-031: Fix asymmetric Flickable padding after card resize (post-implementation bugfix)
  - REQs: REQ-F-004, REQ-NF-005
  - Check: `AppearancePage.qml`'s `colorSchemeRow` control wrapper `Item.implicitHeight` matches `ColorSchemeSwatchCard`'s `implicitHeight` (76.8), not the stale pre-resize value (96); mockup comparison confirmed even top/bottom padding around cards in the scroll strip. See DESIGN.md §9 point 4.
