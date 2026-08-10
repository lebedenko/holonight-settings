function(configure_settings_activation_service output_file)
  if(NOT IS_ABSOLUTE "${CMAKE_INSTALL_FULL_BINDIR}")
    message(FATAL_ERROR "CMAKE_INSTALL_FULL_BINDIR must be absolute")
  endif()

  set(HOLONIGHT_SETTINGS_EXECUTABLE "${CMAKE_INSTALL_FULL_BINDIR}/holonight-settings")
  configure_file(
      "${PROJECT_SOURCE_DIR}/data/dbus-1/services/org.holonight.Settings.service.in"
      "${output_file}"
      @ONLY)
endfunction()
