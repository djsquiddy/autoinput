
find_program(CLANG_FORMAT_EXE NAMES clang-format)

if(CLANG_FORMAT_EXE)
    file(GLOB_RECURSE AUTOINPUT_FORMAT_SOURCES CONFIGURE_DEPENDS
            "${CMAKE_SOURCE_DIR}/src/*.cpp"
            "${CMAKE_SOURCE_DIR}/src/*.h"
            "${CMAKE_SOURCE_DIR}/src/*.hpp"
            "${CMAKE_SOURCE_DIR}/tests/*.cpp"
            "${CMAKE_SOURCE_DIR}/tests/*.h"
            "${CMAKE_SOURCE_DIR}/tests/*.hpp"
    )

    add_custom_target(format
            COMMAND "${CLANG_FORMAT_EXE}" -i ${AUTOINPUT_FORMAT_SOURCES}
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            COMMENT "Formatting C++ sources with clang-format"
            VERBATIM
    )

    add_custom_target(format-check
            COMMAND "${CLANG_FORMAT_EXE}" --dry-run --Werror ${AUTOINPUT_FORMAT_SOURCES}
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            COMMENT "Checking C++ formatting with clang-format"
            VERBATIM
    )
endif()