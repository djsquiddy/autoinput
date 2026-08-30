#==========================================================
# Configure imnodes (Optional third-party dependency)
# Project URL: https://github.com/Nelarius/imnodes
# License MIT: https://github.com/Nelarius/imnodes/blob/master/LICENSE.md
#==========================================================
if(AUTOINPUT_UI_WITH_IMNODES)
    set(IMNODES_SEARCH_PATHS
            ${IMNODES_DIR}
            $ENV{IMNODES_DIR}
            "${CMAKE_SOURCE_DIR}/third_party/imnodes"
            "${CMAKE_SOURCE_DIR}/extern/imnodes"
    )

    find_path(IMNODES_INCLUDE_DIR
            NAMES imnodes.h
            PATHS ${IMNODES_SEARCH_PATHS}
            NO_DEFAULT_PATH
    )

    find_file(IMNODES_SOURCE_FILE
            NAMES imnodes.cpp
            PATHS ${IMNODES_SEARCH_PATHS} ${IMNODES_INCLUDE_DIR}
            NO_DEFAULT_PATH
    )

    if(IMNODES_INCLUDE_DIR AND IMNODES_SOURCE_FILE)
        message(STATUS "Found imnodes: include=${IMNODES_INCLUDE_DIR}, source=${IMNODES_SOURCE_FILE}")
        set(IMNODES_FOUND TRUE CACHE INTERNAL "imnodes found")
        set(IMNODES_SOURCES "${IMNODES_SOURCE_FILE}" CACHE INTERNAL "imnodes source file")
        set(IMNODES_INCLUDE_DIR "${IMNODES_INCLUDE_DIR}" CACHE INTERNAL "imnodes include directory")
    else()
        message(FATAL_ERROR
            "AUTOINPUT_UI_WITH_IMNODES is ON, but imnodes was not found.\n"
            "Please provide imnodes by placing it into 'third_party/imnodes' or 'extern/imnodes', "
            "or specify -DIMNODES_DIR=/path/to/imnodes."
        )
    endif()
endif()