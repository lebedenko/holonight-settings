cmake_minimum_required(VERSION 3.25)
project(acf005-package-consumer LANGUAGES CXX)

find_package(HoloNightConfig REQUIRED)
find_package(HoloNightShellConfig REQUIRED)
find_package(HolonightQt REQUIRED)

add_executable(acf005-package-consumer main.cpp)
target_compile_features(acf005-package-consumer PRIVATE cxx_std_23)
target_link_libraries(acf005-package-consumer PRIVATE HoloNight::Config HoloNightShellConfig::Config HolonightQt::Appearance)
