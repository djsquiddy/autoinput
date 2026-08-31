#==========================================================
# Configure Documentation Generation
# Configures Doxyfile and registers the 'docs' custom target
#==========================================================

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(DOXYGEN_IN "${PROJECT_ROOT_DIR}/docs/Doxyfile.in")
set(DOXYGEN_OUT "${CMAKE_BINARY_DIR}/Doxyfile")
set(DOXYGEN_OUTPUT_DIR "${PROJECT_ROOT_DIR}/docs/doxygen")
set(GEN_DOCS_SCRIPT "${PYTHON_COMMANDS_DIR}/gen_docs.py")

if(EXISTS "${DOXYGEN_IN}")
    configure_file("${DOXYGEN_IN}" "${DOXYGEN_OUT}" @ONLY)
endif()

add_custom_target(docs
    COMMAND
        ${CMAKE_COMMAND} -E env
        "PYTHONPATH=${SCRIPTS_DIR}"
        "${Python3_EXECUTABLE}"
        "${GEN_DOCS_SCRIPT}"
        --src-dir "${PROJECT_ROOT_DIR}/src"
        --output-dir "${PROJECT_ROOT_DIR}/docs/api"
        --config "${DOXYGEN_OUT}"
    WORKING_DIRECTORY "${PROJECT_ROOT_DIR}"
    COMMENT "Generating API documentation (Doxygen with Markdown fallback)"
    VERBATIM
)
