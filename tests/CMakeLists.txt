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

if(BUILD_APP)
  holonight_add_test_exe(test_holonight_settings
    main.cpp
    test_acf005.cpp
    test_acf005_qml_contract.cpp
    test_settings_activation.cpp
    ../apps/settings/src/AppearanceEditModel.cpp
    ../apps/settings/src/ShellSettingsEditModel.cpp
    ../apps/settings/src/AppearanceFileService.cpp
    ../apps/settings/src/ShellConfigFileService.cpp
    ../apps/settings/src/SettingsSaveCoordinator.cpp
    ../apps/settings/src/FileRevision.cpp
    ../apps/settings/src/theme_swatch_tokens.cpp
    ../apps/settings/src/ShellStatusService.cpp
    ../apps/settings/src/FontListModel.cpp
    ../apps/settings/src/SettingsActivationService.cpp
  )
  target_include_directories(test_holonight_settings PRIVATE ${CMAKE_SOURCE_DIR}/apps/settings/src)
  target_link_libraries(test_holonight_settings PRIVATE
    HoloNight::Config HoloNightShellConfig::Config HolonightQt::Appearance HolonightQt::Theme
    GTest::gtest GTest::gmock Qt6::Gui Qt6::DBus
    Qt6::Qml Qt6::Quick Qt6::Test)
endif()

add_test(NAME acf005_package_consumer
  COMMAND ${CMAKE_COMMAND}
    -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/package-consumer
    -DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/package-consumer-build
    -DPROVIDER_PREFIX=/tmp/holonight-provider-prefix
    -P ${CMAKE_CURRENT_SOURCE_DIR}/run_package_consumer.cmake)

add_test(NAME acf005_clean_break
  COMMAND ${CMAKE_COMMAND}
    -DPROJECT_ROOT=${CMAKE_SOURCE_DIR}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/check_acf005_clean_break.cmake)

if(ENABLE_COVERAGE)
  foreach(test_exe IN ITEMS test_holonight_settings)
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
    DEPENDS test_holonight_settings)
endif()
