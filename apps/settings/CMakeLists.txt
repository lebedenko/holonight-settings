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
    qml/IntegrationsPage.qml
    qml/AudioPage.qml
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

function(add_settings_application target main_source)
  qt_add_executable(${target}
      ${main_source}
      src/SettingsApplication.h
      src/SettingsApplication.cpp
      src/SettingsActivationService.h
      src/SettingsActivationService.cpp
  )

  qt_add_qml_module(${target}
      OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml/${target}/HolonightSettings"
      URI HolonightSettings
      VERSION 1.0
      RESOURCE_PREFIX "/qt/qml"
      NO_IMPORT_SCAN
      QML_FILES ${SETTINGS_QML_FILES}
      SOURCES
          src/AppearanceEditModel.h
          src/AppearanceEditModel.cpp
          src/ShellSettingsEditModel.h
          src/ShellSettingsEditModel.cpp
          src/AppearanceFileService.h
          src/AppearanceFileService.cpp
          src/AppearanceAdapterClient.h
          src/AppearanceAdapterClient.cpp
          src/ShellConfigFileService.h
          src/ShellConfigFileService.cpp
          src/SettingsSaveCoordinator.h
          src/SettingsSaveCoordinator.cpp
          src/FileRevision.h
          src/FileRevision.cpp
          src/theme_swatch_tokens.h
          src/theme_swatch_tokens.cpp
          src/FontListModel.h
          src/FontListModel.cpp
          src/ShellStatusService.h
          src/ShellStatusService.cpp
          src/AudioControllerQml.h
          src/AudioControllerQml.cpp
  )

  file(GLOB SETTINGS_NAVIGATION_ICONS CONFIGURE_DEPENDS
      "${PROJECT_SOURCE_DIR}/assets/settings-navigation/*.svg"
  )
  qt6_add_resources(${target} "settings_navigation_icons"
      PREFIX "/HolonightSettings"
      BASE "${PROJECT_SOURCE_DIR}/assets"
      FILES
          ${SETTINGS_NAVIGATION_ICONS}
          "${PROJECT_SOURCE_DIR}/assets/holonight-settings.svg"
  )

  target_link_libraries(${target} PRIVATE
      HoloNight::Config
      HoloNightShellConfig::Config
      HoloNightSystem::Audio
      HolonightQt::Appearance
      HolonightQt::Theme
      Qt6::Core
      Qt6::Gui
      Qt6::Quick
      Qt6::Qml
      Qt6::DBus
  )

  target_compile_definitions(${target} PRIVATE
      HOLONIGHT_SETTINGS_VERSION="${PROJECT_VERSION}"
  )

  target_include_directories(${target} PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  target_compile_features(${target} PRIVATE cxx_std_23)

  qt_add_resources(${target} controls_config
      PREFIX "/"
      FILES qtquickcontrols2.conf
  )
  target_compile_definitions(${target} PRIVATE
      SETTINGS_BUILD_DIR="$<TARGET_FILE_DIR:${target}>"
      SETTINGS_DEPENDENCY_QML_DIR="${SETTINGS_DEPENDENCY_QML_DIR}"
      SETTINGS_INSTALLED_QML_DIR="${CMAKE_INSTALL_LIBDIR}/qt6/qml"
  )
endfunction()

add_settings_application(holonight-settings src/main.cpp)
if(BUILD_TESTS)
  add_settings_application(settings_controls_acceptance "${PROJECT_SOURCE_DIR}/tests/settings_controls_acceptance.cpp")
  target_link_libraries(settings_controls_acceptance PRIVATE Qt6::Test)
endif()

install(TARGETS holonight-settings DESTINATION ${CMAKE_INSTALL_BINDIR})
install(
    FILES ${PROJECT_SOURCE_DIR}/assets/holonight-settings.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
)
install(
    FILES ${PROJECT_SOURCE_DIR}/data/applications/org.holonight.Settings.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)
include(${PROJECT_SOURCE_DIR}/cmake/ConfigureSettingsActivation.cmake)
configure_settings_activation_service(${CMAKE_CURRENT_BINARY_DIR}/org.holonight.Settings.service)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/org.holonight.Settings.service
    DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/services
)
