add_library(holonight_config STATIC
    src/ConfigParsers.cpp
    src/ConfigWriter.cpp
)

add_library(HolonightConfig::Config ALIAS holonight_config)
set_target_properties(holonight_config PROPERTIES EXPORT_NAME Config)

target_include_directories(holonight_config
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(holonight_config PUBLIC
    Qt6::Core
    PkgConfig::TOMLPLUSPLUS
)

target_compile_features(holonight_config PUBLIC cxx_std_23)

include(CMakePackageConfigHelpers)

install(TARGETS holonight_config
  EXPORT HolonightConfigTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT HolonightConfigTargets
  FILE HolonightConfigTargets.cmake
  NAMESPACE HolonightConfig::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/HolonightConfig
)
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/HolonightConfigConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/HolonightConfigConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/HolonightConfig
)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/HolonightConfigConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/HolonightConfigConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/HolonightConfigConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/HolonightConfig
)
