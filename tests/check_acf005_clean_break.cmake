file(GLOB_RECURSE production_files LIST_DIRECTORIES false
  "${PROJECT_ROOT}/apps/*"
  "${PROJECT_ROOT}/libs/*")

set(forbidden_patterns
  "theme[.]conf"
  "appearance[.]json"
  "(^|[^A-Za-z])ThemeConfigFile([^A-Za-z]|$)"
  "(^|[^A-Za-z])SettingsEditModel([^A-Za-z]|$)"
  "HolonightConfig::Config"
  "holonight_config/"
  "Save & Apply")

foreach(production_file IN LISTS production_files)
  file(READ "${production_file}" contents)
  foreach(pattern IN LISTS forbidden_patterns)
    if(contents MATCHES "${pattern}")
      message(FATAL_ERROR "Forbidden ACF-005 legacy pattern '${pattern}' in ${production_file}")
    endif()
  endforeach()
endforeach()

if(EXISTS "${PROJECT_ROOT}/libs/holonight-config/CMakeLists.txt")
  message(FATAL_ERROR "The removed local configuration package still exists")
endif()
