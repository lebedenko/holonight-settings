# Shared System Services Adoption

Settings adopts `HoloNightSystem::Audio` from baseline `b480b860424eaf77f5b76f67a9e79f518cdc21f7`.
It creates and starts its own `HoloNight::System::AudioController`, so Audio remains available without Shell.
The first page presents upstream availability, master output controls, and live input/output device models. Detailed
filtering, validation, per-device controls, and stream-routing UX require a separate accepted UI specification.
