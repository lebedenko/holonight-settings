set(SETTINGS_QML_FILES
    qml/SettingsWindow.qml
    qml/NavPanel.qml
    qml/ContentStack.qml
    qml/PlaceholderPage.qml
    qml/FooterBar.qml
    qml/AppearancePage.qml
    qml/ColorSchemeSwatchCard.qml
    qml/EdgeFadeOverlay.qml
    qml/BarPage.qml
    qml/WeatherPage.qml
    qml/PreviewPanel/PreviewPanel.qml
)

foreach(qml_file IN LISTS SETTINGS_QML_FILES)
  cmake_path(ABSOLUTE_PATH qml_file
        BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        NORMALIZE
        OUTPUT_VARIABLE qml_absolute)
  cmake_path(RELATIVE_PATH qml_absolute
        BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/qml"
        OUTPUT_VARIABLE qml_alias)
  set_source_files_properties("${qml_file}" PROPERTIES QT_RESOURCE_ALIAS "${qml_alias}")
endforeach()

qt_add_executable(holonight-settings
    src/main.cpp
    src/SettingsApplication.h
    src/SettingsApplication.cpp
    src/SettingsActivationService.h
    src/SettingsActivationService.cpp
)

qt_add_qml_module(holonight-settings
    URI HolonightSettings
    VERSION 1.0
    RESOURCE_PREFIX "/qt/qml"
    NO_IMPORT_SCAN
    QML_FILES ${SETTINGS_QML_FILES}
    SOURCES
        src/SettingsEditModel.h
        src/SettingsEditModel.cpp
        src/ConfigFileService.h
        src/ConfigFileService.cpp
        src/ThemeConfigFile.h
        src/ThemeConfigFile.cpp
        src/theme_swatch_tokens.h
        src/theme_swatch_tokens.cpp
        src/FontListModel.h
        src/FontListModel.cpp
        src/ShellStatusService.h
        src/ShellStatusService.cpp
)

file(GLOB SETTINGS_NAVIGATION_ICONS CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/assets/settings-navigation/*.svg"
)
qt6_add_resources(holonight-settings "settings_navigation_icons"
    PREFIX "/HolonightSettings"
    BASE "${PROJECT_SOURCE_DIR}/assets"
    FILES
        ${SETTINGS_NAVIGATION_ICONS}
        "${PROJECT_SOURCE_DIR}/assets/holonight-settings.svg"
)

target_link_libraries(holonight-settings PRIVATE
    HolonightConfig::Config
    HolonightQt::Config
    HolonightQt::Theme
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::DBus
)

target_compile_definitions(holonight-settings PRIVATE
    HOLONIGHT_SETTINGS_VERSION="${PROJECT_VERSION}"
)

target_include_directories(holonight-settings PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(holonight-settings PRIVATE cxx_std_23)

install(TARGETS holonight-settings DESTINATION ${CMAKE_INSTALL_BINDIR})
install(
    FILES ${PROJECT_SOURCE_DIR}/assets/holonight-settings.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
)
install(
    FILES ${PROJECT_SOURCE_DIR}/data/applications/org.holonight.Settings.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)
configure_file(
    ${PROJECT_SOURCE_DIR}/data/dbus-1/services/org.holonight.Settings.service.in
    ${CMAKE_CURRENT_BINARY_DIR}/org.holonight.Settings.service
    @ONLY
)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/org.holonight.Settings.service
    DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/services
)
