cmake_minimum_required(VERSION 4.2)
project(autoinput VERSION 0.1.0 LANGUAGES CXX)

set(PROJECT_ROOT_DIR "${CMAKE_SOURCE_DIR}")
set(PYTHON_SCRIPTS_MODULE_NAME "scripts")
set(PYTHON_COMMAND_SCRIPTS_MODULE_NAME "commands")
set(SCRIPTS_DIR "${CMAKE_SOURCE_DIR}/${PYTHON_SCRIPTS_MODULE_NAME}")
set(PYTHON_COMMANDS_DIR "${CMAKE_SOURCE_DIR}/scripts/commands")
set(RESOURCES_DIR "${CMAKE_SOURCE_DIR}/resources")
set(LOCALIZATION_DIR "${RESOURCES_DIR}/localization")
set(CLI_RESOURCES_DIR "${RESOURCES_DIR}/cli")
set(CODE_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")

set(PROJECT_NAME_CORE "${PROJECT_NAME}-core")
set(PROJECT_NAME_TEST "${PROJECT_NAME}-tests")
set(PROJECT_NAME_TRAY "${PROJECT_NAME}-tray")
set(PROJECT_NAME_UI "${PROJECT_NAME}-ui")

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

include(configure_gsl)
include(configure_toml_plusplus)
include(configure_localization)
include(configure_cli_help)
include(configure_app_icon)
include(configure_clang_tidy)

if(AUTOINPUT_UI_WITH_IMNODES)
    include(configure_imnodes)
endif()

add_subdirectory(src)

if(AUTOINPUT_BUILD_TESTS)
    add_subdirectory(tests)
endif()
