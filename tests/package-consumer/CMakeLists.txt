cmake_minimum_required(VERSION 3.25)
project(holonight-config-consumer LANGUAGES CXX)

find_package(HolonightConfig CONFIG REQUIRED)

add_executable(holonight-config-consumer main.cpp)
target_link_libraries(holonight-config-consumer PRIVATE HolonightConfig::Config)
target_compile_features(holonight-config-consumer PRIVATE cxx_std_23)
