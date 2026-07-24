cmake_minimum_required(VERSION 4.2)
project(autoinput LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)
# Enable ExternalProject CMake module
include(ExternalProject)
include(CMakePushCheckState)
include(CheckCXXCompilerFlag)

######################################################################################################################################
# Cmake modules
######################################################################################################################################
# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_MODULE_PATH)
    set(CMAKE_MODULE_PATH)
endif()

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

include(configure_toml_plusplus)
include(configure_googletest)
######################################################################################################################################
# Versioning
######################################################################################################################################
set(VERSION_MAJOR 0 CACHE STRING "Project major version number.")
set(VERSION_MINOR 1 CACHE STRING "Project minor version number.")
set(VERSION_PATCH 0 CACHE STRING "Project patch version number.")

set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
mark_as_advanced(VERSION_MAJOR VERSION_MINOR VERSION_PATCH)
message(STATUS "Processing ${PROJECT_NAME}")
message(STATUS "Setting up version ${VERSION}")
######################################################################################################################################
# Program Executables
######################################################################################################################################
add_executable(${PROJECT_NAME})
######################################################################################################################################
# Program Options
######################################################################################################################################
option(ENABLE_KEYBOARD_HOOK "Enable the keyboard listeners" ON)
option(ENABLE_MOUSE_HOOK "Enable the mouse listeners" ON)
option(ENABLE_FAKE_HOOK "Enable only logging the events instead of actually performing them" OFF)

function(configure_autoinput_target target_name)
    target_compile_definitions(${target_name} PRIVATE CONFIGS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/configs")

    if (ENABLE_KEYBOARD_HOOK)
        target_compile_definitions(${target_name} PRIVATE AUTOINPUT_HOOK_KEYBOARD_ENABLED=1)
    else()
        target_compile_definitions(${target_name} PRIVATE AUTOINPUT_HOOK_KEYBOARD_ENABLED=0)
    endif()

    if (ENABLE_MOUSE_HOOK)
        target_compile_definitions(${target_name} PRIVATE AUTOINPUT_HOOK_MOUSE_ENABLED=1)
    else()
        target_compile_definitions(${target_name} PRIVATE AUTOINPUT_HOOK_MOUSE_ENABLED=0)
    endif()

    if (ENABLE_FAKE_HOOK)
        target_compile_definitions(${target_name} PRIVATE AUTOINPUT_FAKE_HOOK=1)
    else()
        target_compile_definitions(${target_name} PRIVATE AUTOINPUT_FAKE_HOOK=0)
    endif()

    target_compile_definitions(${target_name} PRIVATE CONFIGS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/configs")
    target_compile_definitions(${target_name} PUBLIC TOML_HEADER_ONLY=0)

    if (WIN32)
        target_compile_definitions(${target_name} PRIVATE NOGDI)
    endif()
endfunction()

######################################################################################################################################
# Program Defines
######################################################################################################################################
# Set the configs path macro to the absolute path on the dev machine
target_compile_definitions(${PROJECT_NAME} PUBLIC TOML_HEADER_ONLY=0)

if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE NOGDI)
endif(WIN32)
######################################################################################################################################
# Program Source
######################################################################################################################################
if (NOT PROJECT_SOURCES)
    set(PROJECT_SOURCES)
endif()

if (NOT PROJECT_INCLUDE)
    set(PROJECT_INCLUDE)
endif()

set(PROJECT_SOURCES ${PROJECT_SOURCES}
    src/pch.cpp
    src/autoinput/autoinput.h
    src/autoinput/arguments.cpp
    src/autoinput/arguments.h
    src/autoinput/types.h
    src/autoinput/types.cpp
    src/autoinput/mouse.h
    src/autoinput/keyboard.h
    src/autoinput/keyinfo.h
    src/autoinput/platform.h
    src/autoinput/autoinput.cpp
    src/autoinput/utils.h
    src/autoinput/utils.cpp
    src/autoinput/logger.h
    src/autoinput/logger.cpp
    src/autoinput/config.h
    src/autoinput/config.cpp
)

if (WIN32)
    set(PROJECT_SOURCES ${PROJECT_SOURCES} src/autoinput/win32/autoinput_windows.cpp)
endif(WIN32)


######################################################################################################################################
# Program Defines
######################################################################################################################################
set(PROJECT_INCLUDE ${PROJECT_INCLUDE} src)
target_sources(${PROJECT_NAME} PRIVATE src/autoinput/main.cpp ${PROJECT_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_INCLUDE})
target_link_libraries(${PROJECT_NAME} PRIVATE tomlplusplus::tomlplusplus)
configure_autoinput_target(${PROJECT_NAME})

# Enable precompiled headers
target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)


######################################################################################################################################
# Tests
######################################################################################################################################
enable_testing()
add_executable(${PROJECT_NAME}_tests
        tests/autoinput/keyTest.cpp
        tests/autoinput/typesTest.cpp
        tests/autoinput/mouseTest.cpp
        tests/autoinput/utilsTest.cpp
        tests/autoinput/argumentsTest.cpp
        tests/autoinput/configTest.cpp
        ${PROJECT_SOURCES}
)

target_include_directories(${PROJECT_NAME}_tests PRIVATE ${PROJECT_INCLUDE})

target_link_libraries(${PROJECT_NAME}_tests
    PRIVATE
    GTest::gtest_main
    tomlplusplus::tomlplusplus
)
configure_autoinput_target(${PROJECT_NAME}_tests)

target_precompile_headers(${PROJECT_NAME}_tests PRIVATE src/pch.h)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_tests)
