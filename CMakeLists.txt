cmake_minimum_required(VERSION 4.2)
project(autoinput VERSION 0.1.0 LANGUAGES CXX)

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
include(autoinputFormat)

# Clang-Tidy
find_program(CLANG_TIDY_EXE NAMES clang-tidy)

if(CLANG_TIDY_EXE)
    file(GLOB_RECURSE AUTOINPUT_TIDY_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_SOURCE_DIR}/tests/*.cpp"
    )

    add_custom_target(clang-tidy-check
        COMMAND "${CLANG_TIDY_EXE}"
            ${AUTOINPUT_TIDY_SOURCES}
            -p "${CMAKE_BINARY_DIR}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Running clang-tidy static analysis"
        VERBATIM
    )

    if(AUTOINPUT_ENABLE_CLANG_TIDY)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
        set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    endif()
endif()

set(PROJECT_NAME_TEST "${PROJECT_NAME}-tests")
set(PROJECT_NAME_TRAY "${PROJECT_NAME}-tray")
set(PROJECT_NAME_UI "${PROJECT_NAME}-ui")

include(configure_gsl)
include(configure_toml_plusplus)
include(configure_localization)

add_subdirectory(src)

if(AUTOINPUT_BUILD_TESTS)
    add_subdirectory(tests)
endif()
