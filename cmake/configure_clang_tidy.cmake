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
