#==========================================================
# Configure Localization ID Generation
# Generates localizationIds.h into the CMake build directory
#==========================================================

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(LOCALIZATION_SOURCE_TOML "${CMAKE_SOURCE_DIR}/resources/localization/en-US.toml")
set(GEN_LOCALIZATION_IDS_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/gen_localization_ids.py")
set(LOCALIZATION_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
set(LOCALIZATION_IDS_FILENAME "${LOCALIZATION_GENERATED_DIR}/autoinput_ui/core/localizationIds")
set(LOCALIZATION_IDS_HEADER "${LOCALIZATION_IDS_FILENAME}.h")
set(LOCALIZATION_IDS_SOURCE "${LOCALIZATION_IDS_FILENAME}.cpp")

# Generate during CMake configuration (prebuild step) so files exist immediately for IDE indexing and early build phases
execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${GEN_LOCALIZATION_IDS_SCRIPT}"
        --loc "${LOCALIZATION_SOURCE_TOML}"
        --header "${LOCALIZATION_IDS_HEADER}"
        --source "${LOCALIZATION_IDS_SOURCE}"
    RESULT_VARIABLE GEN_LOC_RES
)

if(NOT GEN_LOC_RES EQUAL 0)
    message(FATAL_ERROR "Failed to generate localization IDs header during CMake configure: ${LOCALIZATION_IDS_HEADER}")
endif()

# Set up custom command and target for incremental build-time regeneration when TOML or script changes
add_custom_command(
    OUTPUT "${LOCALIZATION_IDS_HEADER}" "${LOCALIZATION_IDS_SOURCE}"
    COMMAND "${Python3_EXECUTABLE}" "${GEN_LOCALIZATION_IDS_SCRIPT}"
        --loc "${LOCALIZATION_SOURCE_TOML}"
        --header "${LOCALIZATION_IDS_HEADER}"
        --source "${LOCALIZATION_IDS_SOURCE}"
    DEPENDS
        "${LOCALIZATION_SOURCE_TOML}"
        "${GEN_LOCALIZATION_IDS_SCRIPT}"
        "${CMAKE_SOURCE_DIR}/scripts/utils.py"
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
