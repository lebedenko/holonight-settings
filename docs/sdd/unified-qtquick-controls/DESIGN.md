# Design

## Migration inventory

AppearancePage: direct Holonight and Basic/QQC2; runtime controls and ButtonGroup. Guard Switch sizeRole with
conditional Binding (Large only when supported), matching the provider gallery.
BarPage, WeatherPage, IntegrationsPage: direct Holonight standard controls.
FooterBar: HnStyle Buttons, Basic Dialog and Overlay. ContentStack: Basic StackView and transition enums.
AudioPage already uses Controls and keeps its namespace. Core/composites and custom swatch painting stay explicit.

## Executable and verification architecture

A small CMake function shares production sources, QML inventory and resources with a dedicated acceptance main.
The acceptance main constructs SettingsApplication and finds its actual window through Qt window enumeration;
no production diagnostic interface is added. Page changes use properties, with bounded StackView completion.
The engine adds executable-relative installed QML discovery; configured dependency discovery is conditional on
running from the build executable directory. The embedded root configuration supplies Style=Holonight without
imperative selection. D-Bus activation continues using the configure-time install prefix.

Import policy follows the provider application boundary and has independent positive/negative fixtures.
CTest/lint and package-consumer paths derive from CMake configuration. CI stages dependencies and pins UQC-101;
dependency tests and examples are explicitly disabled. Installed acceptance uses the configured staging prefix.
