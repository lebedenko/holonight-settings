cmake_minimum_required(VERSION 3.25)

if(NOT DEFINED SETTINGS_SOURCE_DIR OR NOT DEFINED TEST_PREFIX OR NOT DEFINED EXPECTED_EXEC)
  message(FATAL_ERROR "SETTINGS_SOURCE_DIR, TEST_PREFIX, and EXPECTED_EXEC are required")
endif()

set(PROJECT_SOURCE_DIR "${SETTINGS_SOURCE_DIR}")
set(CMAKE_INSTALL_PREFIX "${TEST_PREFIX}")
set(CMAKE_INSTALL_BINDIR bin)
set(CMAKE_INSTALL_FULL_BINDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")
include("${SETTINGS_SOURCE_DIR}/cmake/ConfigureSettingsActivation.cmake")

string(MAKE_C_IDENTIFIER "${TEST_PREFIX}" prefix_id)
set(service_file "${CMAKE_CURRENT_BINARY_DIR}/org.holonight.Settings-${prefix_id}.service")
configure_settings_activation_service("${service_file}")

file(READ "${service_file}" service_contents)
if(NOT service_contents MATCHES "Exec=${EXPECTED_EXEC}([\r\n]|$)")
  message(FATAL_ERROR "Expected Exec=${EXPECTED_EXEC}, got:\n${service_contents}")
endif()
