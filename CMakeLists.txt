cmake_minimum_required(VERSION 3.25)
project(holonight-settings VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)

option(BUILD_APP "Build the HoloNight Settings application" ON)
option(BUILD_TESTS "Build unit and integration tests" OFF)
option(ENABLE_COVERAGE "Instrument tests for gcov/gcovr coverage" OFF)

find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(PkgConfig REQUIRED)
pkg_check_modules(TOMLPLUSPLUS REQUIRED IMPORTED_TARGET tomlplusplus)

add_subdirectory(libs/holonight-config)

if(BUILD_APP)
  find_package(Qt6 REQUIRED COMPONENTS Gui Quick Qml DBus)
  find_package(HolonightQt REQUIRED)
  qt_policy(SET QTP0004 NEW)
  add_subdirectory(apps/settings)
endif()

if(BUILD_TESTS)
  enable_testing()
  find_package(Qt6 REQUIRED COMPONENTS Gui DBus Qml Quick Test)
  find_package(HolonightQt REQUIRED)
  find_package(GTest CONFIG REQUIRED)
  add_subdirectory(tests)
endif()

if(ENABLE_COVERAGE)
  if(NOT BUILD_TESTS)
    message(FATAL_ERROR "ENABLE_COVERAGE requires BUILD_TESTS=ON")
  endif()
  find_program(GCOVR gcovr REQUIRED)
  target_compile_options(holonight_config PRIVATE --coverage)
  target_link_options(holonight_config PRIVATE --coverage)
endif()

find_program(CLANG_FORMAT clang-format)
find_program(CLANG_TIDY clang-tidy)
find_program(RUN_CLANG_TIDY run-clang-tidy)
find_program(QMLLINT qmllint HINTS "${Qt6_DIR}/../../../lib/qt6/bin" "/usr/lib/qt6/bin")

file(GLOB_RECURSE ALL_SOURCE_FILES LIST_DIRECTORIES false CONFIGURE_DEPENDS
  apps/*.cpp apps/*.h libs/*.cpp libs/*.h tests/*.cpp tests/*.h)
file(GLOB_RECURSE ALL_CXX_FILES LIST_DIRECTORIES false CONFIGURE_DEPENDS
  apps/*.cpp libs/*.cpp tests/*.cpp)
file(GLOB_RECURSE ALL_QML_FILES LIST_DIRECTORIES false CONFIGURE_DEPENDS apps/*.qml)

if(CLANG_FORMAT)
  add_custom_target(format COMMAND ${CLANG_FORMAT} -i ${ALL_SOURCE_FILES} VERBATIM)
  add_custom_target(format-check COMMAND ${CLANG_FORMAT} --dry-run --Werror ${ALL_SOURCE_FILES} VERBATIM)
endif()

if(CLANG_TIDY)
  if(RUN_CLANG_TIDY)
    add_custom_target(tidy COMMAND ${RUN_CLANG_TIDY} -quiet -p ${CMAKE_BINARY_DIR} ${ALL_CXX_FILES} VERBATIM)
  else()
    add_custom_target(tidy COMMAND ${CLANG_TIDY} --quiet -p ${CMAKE_BINARY_DIR} ${ALL_CXX_FILES} VERBATIM)
  endif()
endif()

if(BUILD_APP AND QMLLINT)
  add_custom_target(qml-lint
    COMMAND ${QMLLINT} --unqualified disable --unused-imports disable
      -I "${CMAKE_CURRENT_BINARY_DIR}/apps/settings"
      -I "${CMAKE_BINARY_DIR}"
      -I "/tmp/holonight-qt-prefix/${CMAKE_INSTALL_LIBDIR}/qt6/qml"
      ${ALL_QML_FILES}
    DEPENDS holonight-settings_qmltyperegistration
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM)
endif()
