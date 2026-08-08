cmake_minimum_required(VERSION 4.2)
project(autoinput VERSION 0.1.0 LANGUAGES CXX)

# Sets Static Runtime: /MTd for Debug, /MT for Release
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(PREPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(ExternalProject)
include(CMakePushCheckState)
include(CheckCXXCompilerFlag)

include(autoinputOptions)
include(autoinputInstall)
include(autoinputTarget)
include(autoinputSources)
include(autoinputPlatform)

include(configure_gsl)
include(configure_toml_plusplus)
include(configure_googletest)

add_subdirectory(src)

if(AUTOINPUT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
