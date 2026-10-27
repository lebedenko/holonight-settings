cmake_minimum_required(VERSION 3.25)
project(holonight-settings VERSION 0.1.0 LANGUAGES CXX)

# Installed QML providers follow the standard CMake prefix search path.
set(_qml_prefixes ${CMAKE_PREFIX_PATH})
if(DEFINED ENV{CMAKE_PREFIX_PATH})
  string(REPLACE ":" ";" _prefix_environment "$ENV{CMAKE_PREFIX_PATH}")
  list(APPEND _qml_prefixes ${_prefix_environment})
endif()
foreach(_prefix IN LISTS _qml_prefixes)
  if(IS_DIRECTORY "${_prefix}/lib/qt6/qml")
    list(APPEND QML_IMPORT_PATH "${_prefix}/lib/qt6/qml")
  endif()
endforeach()
if(DEFINED ENV{QML_IMPORT_PATH})
  string(REPLACE ":" ";" _qml_environment "$ENV{QML_IMPORT_PATH}")
  list(APPEND QML_IMPORT_PATH ${_qml_environment})
endif()
list(REMOVE_DUPLICATES QML_IMPORT_PATH)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
install(FILES LICENSE LICENSES/GPL-3.0-or-later.txt
  DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/${PROJECT_NAME}")

option(BUILD_APP "Build the HoloNight Settings application" ON)
option(BUILD_TESTS "Build unit and integration tests" OFF)
option(ENABLE_COVERAGE "Instrument tests for gcov/gcovr coverage" OFF)

find_package(Qt6 6.11 REQUIRED COMPONENTS Core)
find_package(HoloNightConfig REQUIRED)
find_package(HoloNightShellConfig 0.2 REQUIRED)
find_package(HoloNightSystemServices REQUIRED COMPONENTS Audio Compositor)

if(BUILD_APP OR BUILD_TESTS)
  find_package(HolonightQt REQUIRED)
  get_filename_component(_holonight_lib_dir "${HolonightQt_DIR}/../.." ABSOLUTE)
  set(SETTINGS_DEPENDENCY_QML_DIR "${_holonight_lib_dir}/qt6/qml" CACHE PATH
    "Installed provider QML used by build executables and checks")
endif()

if(BUILD_APP)
  find_package(Qt6 6.11 REQUIRED COMPONENTS Gui Quick Qml DBus Network)
  find_package(HolonightQt REQUIRED)
  if(BUILD_TESTS)
    find_package(Qt6 6.11 REQUIRED COMPONENTS Test)
  endif()
  qt_policy(SET QTP0004 NEW)
  add_subdirectory(apps/settings)
endif()

if(ENABLE_COVERAGE)
  if(NOT BUILD_TESTS)
    message(FATAL_ERROR "ENABLE_COVERAGE requires BUILD_TESTS=ON")
  endif()
  find_program(GCOVR gcovr REQUIRED)
endif()

if(BUILD_TESTS)
  enable_testing()
  find_package(Qt6 6.11 REQUIRED COMPONENTS Gui DBus Qml Quick Test)
  find_package(HolonightQt REQUIRED)
  find_package(GTest 1.14 CONFIG REQUIRED)
  if(NOT TARGET GTest::gtest OR NOT TARGET GTest::gmock)
    message(FATAL_ERROR "Installed GTest must export GTest::gtest and GTest::gmock")
  endif()
  add_subdirectory(tests)
endif()

find_program(CLANG_FORMAT clang-format)
if(NOT QMLLINT AND TARGET Qt6::qmllint)
  get_target_property(QMLLINT Qt6::qmllint IMPORTED_LOCATION)
endif()

file(GLOB_RECURSE ALL_SOURCE_FILES LIST_DIRECTORIES false CONFIGURE_DEPENDS
  apps/*.cpp apps/*.h libs/*.cpp libs/*.h tests/*.cpp tests/*.h)
# The standalone installed-provider consumer has its own compiler database/check.
file(GLOB_RECURSE ALL_QML_FILES LIST_DIRECTORIES false CONFIGURE_DEPENDS apps/*.qml)

if(CLANG_FORMAT)
  add_custom_target(format COMMAND ${CLANG_FORMAT} -i ${ALL_SOURCE_FILES} VERBATIM)
  add_custom_target(format-check COMMAND ${CLANG_FORMAT} --dry-run --Werror ${ALL_SOURCE_FILES} VERBATIM)
endif()

# Use the same direct compilation-database check as task tidy.
add_custom_target(tidy
  COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/scripts/quality.py" tidy --build "${CMAKE_BINARY_DIR}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  VERBATIM
)

if(BUILD_APP AND QMLLINT)
  add_custom_target(qml-lint
    COMMAND ${QMLLINT} --unqualified disable --unused-imports disable
      -I "${CMAKE_CURRENT_BINARY_DIR}/apps/settings/qml/holonight-settings"
      -I "${CMAKE_BINARY_DIR}"
      -I "${SETTINGS_DEPENDENCY_QML_DIR}"
      ${ALL_QML_FILES}
    DEPENDS holonight-settings_qmltyperegistration
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM)
endif()
