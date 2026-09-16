# Settings dropdown composition consistency

Status: Investigation complete; implementation Planned, not assigned.
Date: 2026-09-17.

User-directed application-local follow-up, explicitly non-blocking for the
unified Qt Quick Controls initiative. Current request is to identify the
Appearance/Weather implementation difference, not implement a repair.

## Observed behavior and baseline

On Sway at measured DPR 1.25 with Fusion, the user reports Appearance dropdowns
look like HoloNight apart from a small filled-triangle indicator. Weather
dropdowns look fully Fusion. Default-style checks passed; the visual difference
is explicitly accepted as non-blocking. Evidence was reviewed by the umbrella
from `sway-adk_476k`, runs `settings-1789596069053077199` and
`settings-1789596751444043230`, both exit 0 with staged loading verified.

Inspected Settings `2508635351e4901e1b03daf62dbb8ef6538ffcc5` and provider
`019d22fd20722a9f119953c1d810fcdbc105d5b2`, matching the immutable acceptance kit.

## Confirmed implementation difference

| Page | Composition | Effect under Fusion |
|---|---|---|
| Appearance | Four `HnIconComboBox` instances for interface, monospace, title and display fonts; each consumes `FontListModel` with `textRole: "display"` and retains the selected family | Shared composite intentionally replaces much of the standard control's presentation |
| Weather | Six ordinary `Controls.ComboBox` instances for provider, location source, temperature, wind, pressure and refresh interval | Models/roles/index/activation/width are configured locally; content, background, delegate and popup presentation are left to the runtime style |

Sources: `apps/settings/qml/AppearancePage.qml` (instances at lines 270, 339,
406 and 460) and `apps/settings/qml/WeatherPage.qml` (95, 165, 234, 272, 311, 449)
at the inspected revision. Shared implementation:
`holonight-qt/qml/controls/HnIconComboBox.qml` in the sibling provider repository.

`HnIconComboBox` derives from runtime-selected `C.ComboBox`; it does not directly
import the HoloNight or Basic style. It supplies its own palette, font/metrics,
`T.TextField` content item, rectangle background, `C.Popup` with a ListView and
HoloNight background, and custom `C.ItemDelegate` backgrounds/icon composition.
It also supplies popup geometry and selection/hover behavior. It does **not**
replace `indicator`, so that part remains inherited from the runtime-selected
ComboBox implementation. This accounts for the reported Fusion triangle while
the surrounding composite retains HoloNight visuals. Some subparts still inherit
style behavior; this is not a claim that every pixel apart from the indicator is
identical in all states.

Observer evidence contains `HnIconComboBox.qml` objects alongside the selected
HoloNight/Fusion ComboBox origins and Appearance/Weather page objects. Both
processes measure DPR 1.25. No targeted TypeError, ReferenceError, binding-loop
or engine-load-failure matches were found. Source inspection establishes the
composition boundary; appearance alone is not treated as proof of style bypass.

## Ownership and follow-up scope

Settings owns the inconsistent page-level choice between standard control and
shared composite. The provider owns the composite implementation, whose retained
HoloNight visuals are allowed by the existing shared-controls contract. This
observation does not justify changing the shared composite or its public API.

A future local change must first choose the desired application presentation:
use runtime-style visuals consistently, or deliberately adopt composite visuals
on both pages. No choice or implementation is accepted by this investigation.
Prefer a Settings-only composition change if sufficient. Preserve retained font
families, fixed-pitch filtering, model roles, current selection, edit-model
bindings, keyboard traversal and popup geometry; do not trade those for visual
consistency. Any demonstrated provider requirement needs its own bounded scope.

## Tasks and eventual verification

- [x] Compare both pages and trace the shared composite's overrides/inherited parts.
- [x] Record source revisions and correlate the reported style/DPR evidence.
- [x] Keep the observation non-blocking and outside UQC repair/integration gates.
- [ ] Agree the desired application-local dropdown presentation.
- [ ] Implement the agreed Settings-only change with a focused behavioral regression.
- [ ] Verify both styles at scales 1/1.25, font retention/filtering, editing and
      first/last popup-row reachability; run relevant Settings policy/acceptance checks.

Investigation changes documentation only. No production edits, product-test
reruns, new manual comparisons or provider repair are requested for this finding.
