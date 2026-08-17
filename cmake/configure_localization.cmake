#==========================================================
# Configure Localization ID Generation
# Generates localizationIds.h into the CMake build directory
#==========================================================

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(LOCALIZATION_SOURCE_TOML "${LOCALIZATION_DIR}/en-US.toml")
set(GEN_LOCALIZATION_IDS_MODULE "gen_localization_ids")
set(GEN_LOCALIZATION_IDS_SCRIPT "${SCRIPTS_DIR}/${GEN_LOCALIZATION_IDS_MODULE}.py")
set(LOCALIZATION_IDS_FILENAME "${CODE_GENERATED_DIR}/autoinput/support/localizationIds")
set(LOCALIZATION_IDS_HEADER "${LOCALIZATION_IDS_FILENAME}.h")
set(LOCALIZATION_IDS_SOURCE "${LOCALIZATION_IDS_FILENAME}.cpp")

# Generate during CMake configuration (prebuild step) so files exist immediately for IDE indexing and early build phases
execute_process(
    COMMAND "${Python3_EXECUTABLE}" -m "${PYTHON_SCRIPTS_MODULE_NAME}.${GEN_LOCALIZATION_IDS_MODULE}"
        --loc "${LOCALIZATION_SOURCE_TOML}"
        --header "${LOCALIZATION_IDS_HEADER}"
        --source "${LOCALIZATION_IDS_SOURCE}"
    WORKING_DIRECTORY "${PROJECT_ROOT_DIR}"
    RESULT_VARIABLE GEN_LOC_RES
)

if(NOT GEN_LOC_RES EQUAL 0)
    message(FATAL_ERROR "Failed to generate localization IDs header during CMake configure: ${LOCALIZATION_IDS_HEADER}, ${LOCALIZATION_IDS_SOURCE}")
endif()

# Set up custom command and target for incremental build-time regeneration when TOML or script changes
add_custom_command(
    OUTPUT "${LOCALIZATION_IDS_HEADER}" "${LOCALIZATION_IDS_SOURCE}"
    COMMAND "${Python3_EXECUTABLE}" -m "${PYTHON_SCRIPTS_MODULE_NAME}.${GEN_LOCALIZATION_IDS_MODULE}"
        --loc "${LOCALIZATION_SOURCE_TOML}"
        --header "${LOCALIZATION_IDS_HEADER}"
        --source "${LOCALIZATION_IDS_SOURCE}"
    DEPENDS
        "${LOCALIZATION_SOURCE_TOML}"
        "${GEN_LOCALIZATION_IDS_SCRIPT}"
        "${SCRIPTS_DIR}/utils.py"
    WORKING_DIRECTORY "${PROJECT_ROOT_DIR}"
    COMMENT "Generating localization IDs header: ${LOCALIZATION_IDS_HEADER}, source: ${LOCALIZATION_IDS_SOURCE}"
    VERBATIM
)

add_custom_target(generate_localization_ids
    DEPENDS "${LOCALIZATION_IDS_HEADER}" "${LOCALIZATION_IDS_SOURCE}"
)

source_group("Generated\\Localization" FILES
        ${LOCALIZATION_IDS_HEADER}
        ${LOCALIZATION_IDS_SOURCE}
)
