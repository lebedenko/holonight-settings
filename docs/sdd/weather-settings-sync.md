# Live weather settings

The shared `[weather]` contract is documented in the [umbrella integration note](../../../docs/initiatives/weather-settings-sync.md).

Settings retains DocumentEditSession for Save, watching, three-way conflict handling and minimal
TOML patches. Optional coordinate properties use invalid QVariant for absence and doubles for
present values, including zero. DocumentEditSession treats an absent control value as an absent
override. City selection updates city, country and coordinates before emitting changes, so the
existing document capture sees the complete selection.

WeatherLocationSearch belongs to the executable and uses Qt Network. Pressing Enter searches;
selecting a city stages its coordinates. A numeric latitude/longitude pair stages directly after
finite/range validation. Search text, loading, empty and error states never replace the staged
location. Editing the query cancels old searches, and disconnected reply callbacks cannot publish
superseded results. Manual and automatic values remain staged when their controls are hidden.
Controls use user-action signals to keep projection refreshes from producing new edits.

Acceptance: temporary documents exercise selection, zero/absent coordinates, legacy units,
round trips, resets, external reloads and conflicts. Fixture network replies exercise success,
empty/error states and responses arriving after cancellation.
