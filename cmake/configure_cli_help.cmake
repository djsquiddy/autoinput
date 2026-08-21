#==========================================================
# Configure CLI Help Metadata Generation
# Generates cliHelpMetadata.h/.cpp into the CMake build directory
#==========================================================

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(CLI_HELP_SOURCE_TOML "${CLI_RESOURCES_DIR}/help.toml")
set(GEN_CLI_HELP_MODULE "gen_cli_help")
set(GEN_CLI_HELP_SCRIPT "${SCRIPTS_DIR}/${GEN_CLI_HELP_MODULE}.py")
set(CLI_HELP_METADATA_FILENAME "${CODE_GENERATED_DIR}/autoinput/cli/cliHelpMetadata")
set(CLI_HELP_METADATA_HEADER "${CLI_HELP_METADATA_FILENAME}.h")
set(CLI_HELP_METADATA_SOURCE "${CLI_HELP_METADATA_FILENAME}.cpp")

# Generate during CMake configuration (prebuild step) so files exist immediately for IDE indexing and early build phases
execute_process(
    COMMAND "${Python3_EXECUTABLE}" -m "${PYTHON_SCRIPTS_MODULE_NAME}.${GEN_CLI_HELP_MODULE}"
        --toml "${CLI_HELP_SOURCE_TOML}"
        --header "${CLI_HELP_METADATA_HEADER}"
        --source "${CLI_HELP_METADATA_SOURCE}"
    WORKING_DIRECTORY "${PROJECT_ROOT_DIR}"
    RESULT_VARIABLE GEN_CLI_HELP_RES
)

if(NOT GEN_CLI_HELP_RES EQUAL 0)
    message(FATAL_ERROR "Failed to generate CLI help metadata during CMake configure: ${CLI_HELP_METADATA_HEADER}, ${CLI_HELP_METADATA_SOURCE}")
endif()

# Set up custom command and target for incremental build-time regeneration when TOML or script changes
add_custom_command(
    OUTPUT "${CLI_HELP_METADATA_HEADER}" "${CLI_HELP_METADATA_SOURCE}"
    COMMAND "${Python3_EXECUTABLE}" -m "${PYTHON_SCRIPTS_MODULE_NAME}.${GEN_CLI_HELP_MODULE}"
        --toml "${CLI_HELP_SOURCE_TOML}"
        --header "${CLI_HELP_METADATA_HEADER}"
        --source "${CLI_HELP_METADATA_SOURCE}"
    DEPENDS
        "${CLI_HELP_SOURCE_TOML}"
        "${GEN_CLI_HELP_SCRIPT}"
        "${SCRIPTS_DIR}/cli_help.py"
        "${SCRIPTS_DIR}/utils.py"
    WORKING_DIRECTORY "${PROJECT_ROOT_DIR}"
    COMMENT "Generating CLI help metadata header: ${CLI_HELP_METADATA_HEADER}, source: ${CLI_HELP_METADATA_SOURCE}"
    VERBATIM
)

add_custom_target(generate_cli_help_metadata
    DEPENDS "${CLI_HELP_METADATA_HEADER}" "${CLI_HELP_METADATA_SOURCE}"
)

source_group("Generated\\CliHelp" FILES
        ${CLI_HELP_METADATA_HEADER}
        ${CLI_HELP_METADATA_SOURCE}
)
