include(GoogleTest)

function(holonight_add_test_exe name)
  add_executable(${name} ${ARGN})
  set_target_properties(${name} PROPERTIES AUTOMOC ON)
  target_compile_features(${name} PRIVATE cxx_std_23)
  target_compile_definitions(${name} PRIVATE TEST_SOURCE_DIR="${CMAKE_SOURCE_DIR}" HOLONIGHT_TESTS)
  gtest_discover_tests(${name}
    DISCOVERY_MODE PRE_TEST
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QML_IMPORT_PATH=/tmp/holonight-qt-prefix/lib/qt6/qml;QML2_IMPORT_PATH=/tmp/holonight-qt-prefix/lib/qt6/qml")
endfunction()

holonight_add_test_exe(test_holonight_config
  main.cpp
  test_config_parsers.cpp
  test_configwriter_weather_preservation.cpp
)
target_link_libraries(test_holonight_config PRIVATE
  HolonightConfig::Config GTest::gtest GTest::gmock Qt6::Gui Qt6::Test)

if(BUILD_APP)
  holonight_add_test_exe(test_holonight_settings
    main.cpp
    test_settings_app.cpp
    test_settings_activation.cpp
    ../apps/settings/src/SettingsEditModel.cpp
    ../apps/settings/src/ConfigFileService.cpp
    ../apps/settings/src/ThemeConfigFile.cpp
    ../apps/settings/src/theme_swatch_tokens.cpp
    ../apps/settings/src/ShellStatusService.cpp
    ../apps/settings/src/FontListModel.cpp
    ../apps/settings/src/SettingsActivationService.cpp
  )
  target_include_directories(test_holonight_settings PRIVATE ${CMAKE_SOURCE_DIR}/apps/settings/src)
  target_link_libraries(test_holonight_settings PRIVATE
    HolonightConfig::Config HolonightQt::Config HolonightQt::Theme
    GTest::gtest GTest::gmock Qt6::Gui Qt6::DBus
    Qt6::Qml Qt6::Quick Qt6::Test)
endif()

if(ENABLE_COVERAGE)
  foreach(test_exe IN ITEMS test_holonight_config test_holonight_settings)
    if(TARGET ${test_exe})
      target_compile_options(${test_exe} PRIVATE --coverage)
      target_link_options(${test_exe} PRIVATE --coverage)
    endif()
  endforeach()
  add_custom_target(coverage
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/coverage
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMAND ${GCOVR} --root ${CMAKE_SOURCE_DIR}
      --filter "${CMAKE_SOURCE_DIR}/apps/settings/"
      --filter "${CMAKE_SOURCE_DIR}/libs/holonight-config/"
      --html-details ${CMAKE_BINARY_DIR}/coverage/index.html
      --exclude-unreachable-branches --print-summary
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS test_holonight_config test_holonight_settings)
endif()
