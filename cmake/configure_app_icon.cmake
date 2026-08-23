#==========================================================
# Configure App Icon Generation
# Generates appIcon.ico and appIcon.rc into the CMake build directory
#==========================================================

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(APP_ICON_SOURCE_PNG "${RESOURCES_DIR}/appIcon.png")
set(GEN_APP_ICON_MODULE "gen_app_icon")
set(GEN_APP_ICON_SCRIPT "${PYTHON_COMMANDS_DIR}/${GEN_APP_ICON_MODULE}.py")
set(APP_ICON_OUTPUT_ICO "${CODE_GENERATED_DIR}/autoinput/resources/appIcon.ico")
set(APP_ICON_OUTPUT_RC "${CODE_GENERATED_DIR}/autoinput/resources/appIcon.rc")

# Generate during CMake configuration (prebuild step) so files exist immediately for IDE indexing and early build phases
execute_process(
    COMMAND
        ${CMAKE_COMMAND} -E env
        "PYTHONPATH=${SCRIPTS_DIR}"
        "${Python3_EXECUTABLE}"
        "${GEN_APP_ICON_SCRIPT}"
        --png "${APP_ICON_SOURCE_PNG}"
        --ico "${APP_ICON_OUTPUT_ICO}"
        --rc "${APP_ICON_OUTPUT_RC}"
    WORKING_DIRECTORY "${PROJECT_ROOT_DIR}"
    RESULT_VARIABLE GEN_APP_ICON_RES
)

if(NOT GEN_APP_ICON_RES EQUAL 0)
    message(FATAL_ERROR "Failed to generate app icon during CMake configure: ${APP_ICON_OUTPUT_ICO}, ${APP_ICON_OUTPUT_RC}")
endif()

# Set up custom command and target for incremental build-time regeneration when PNG or script changes
add_custom_command(
    OUTPUT "${APP_ICON_OUTPUT_ICO}" "${APP_ICON_OUTPUT_RC}"
    COMMAND
        ${CMAKE_COMMAND} -E env
        "PYTHONPATH=${SCRIPTS_DIR}"
        "${Python3_EXECUTABLE}"
        "${GEN_APP_ICON_SCRIPT}"
        --png "${APP_ICON_SOURCE_PNG}"
        --ico "${APP_ICON_OUTPUT_ICO}"
        --rc "${APP_ICON_OUTPUT_RC}"
    DEPENDS
        "${APP_ICON_SOURCE_PNG}"
        "${GEN_APP_ICON_SCRIPT}"
        "${SCRIPTS_DIR}/autoinput_tools/icon/generate.py"
        "${SCRIPTS_DIR}/autoinput_tools/icon/ico.py"
        "${SCRIPTS_DIR}/autoinput_tools/icon/rc.py"
        "${SCRIPTS_DIR}/autoinput_tools/file_io.py"
        "${SCRIPTS_DIR}/autoinput_tools/paths.py"
    WORKING_DIRECTORY "${PROJECT_ROOT_DIR}"
    COMMENT "Generating app icon: ${APP_ICON_OUTPUT_ICO}, RC: ${APP_ICON_OUTPUT_RC}"
    VERBATIM
)

add_custom_target(generate_app_icon
    DEPENDS "${APP_ICON_OUTPUT_ICO}" "${APP_ICON_OUTPUT_RC}"
)

source_group("Generated\\Resources" FILES
        ${APP_ICON_OUTPUT_ICO}
        ${APP_ICON_OUTPUT_RC}
)
