#==========================================================
# Configure imgui-node-editor (Optional third-party dependency)
# Project URL: https://github.com/thedmd/imgui-node-editor
# License: https://github.com/thedmd/imgui-node-editor/blob/master/LICENSE
#==========================================================
if(AUTOINPUT_UI_WITH_IMGUI_NODE_EDITOR)
    set(IMGUI_NODE_EDITOR_SEARCH_PATHS
            ${IMGUI_NODE_EDITOR_DIR}
            $ENV{IMGUI_NODE_EDITOR_DIR}
            "${CMAKE_SOURCE_DIR}/third_party/imgui-node-editor"
            "${CMAKE_SOURCE_DIR}/extern/imgui-node-editor"
    )

    find_path(IMGUI_NODE_EDITOR_INCLUDE_DIR
            NAMES imgui_node_editor.h
            PATHS ${IMGUI_NODE_EDITOR_SEARCH_PATHS}
            NO_DEFAULT_PATH
    )

    find_file(IMGUI_NODE_EDITOR_SOURCE_MAIN
            NAMES imgui_node_editor.cpp
            PATHS ${IMGUI_NODE_EDITOR_SEARCH_PATHS} ${IMGUI_NODE_EDITOR_INCLUDE_DIR}
            NO_DEFAULT_PATH
    )

    find_file(IMGUI_NODE_EDITOR_SOURCE_API
            NAMES imgui_node_editor_api.cpp
            PATHS ${IMGUI_NODE_EDITOR_SEARCH_PATHS} ${IMGUI_NODE_EDITOR_INCLUDE_DIR}
            NO_DEFAULT_PATH
    )

    find_file(IMGUI_NODE_EDITOR_SOURCE_CANVAS
            NAMES imgui_canvas.cpp
            PATHS ${IMGUI_NODE_EDITOR_SEARCH_PATHS} ${IMGUI_NODE_EDITOR_INCLUDE_DIR}
            NO_DEFAULT_PATH
    )

    find_file(IMGUI_NODE_EDITOR_SOURCE_CRUDE
            NAMES crude_json.cpp
            PATHS ${IMGUI_NODE_EDITOR_SEARCH_PATHS} ${IMGUI_NODE_EDITOR_INCLUDE_DIR}
            NO_DEFAULT_PATH
    )

    if(IMGUI_NODE_EDITOR_INCLUDE_DIR AND IMGUI_NODE_EDITOR_SOURCE_MAIN)
        set(IMGUI_NODE_EDITOR_SRCS "${IMGUI_NODE_EDITOR_SOURCE_MAIN}")
        if(IMGUI_NODE_EDITOR_SOURCE_API)
            list(APPEND IMGUI_NODE_EDITOR_SRCS "${IMGUI_NODE_EDITOR_SOURCE_API}")
        endif()
        if(IMGUI_NODE_EDITOR_SOURCE_CANVAS)
            list(APPEND IMGUI_NODE_EDITOR_SRCS "${IMGUI_NODE_EDITOR_SOURCE_CANVAS}")
        endif()
        if(IMGUI_NODE_EDITOR_SOURCE_CRUDE)
            list(APPEND IMGUI_NODE_EDITOR_SRCS "${IMGUI_NODE_EDITOR_SOURCE_CRUDE}")
        endif()

        message(STATUS "Found imgui-node-editor: include=${IMGUI_NODE_EDITOR_INCLUDE_DIR}")
        set(IMGUI_NODE_EDITOR_FOUND TRUE CACHE INTERNAL "imgui-node-editor found")
        set(IMGUI_NODE_EDITOR_SOURCES ${IMGUI_NODE_EDITOR_SRCS} CACHE INTERNAL "imgui-node-editor source files")
        set(IMGUI_NODE_EDITOR_INCLUDE_DIR "${IMGUI_NODE_EDITOR_INCLUDE_DIR}" CACHE INTERNAL "imgui-node-editor include directory")
    else()
        message(FATAL_ERROR
            "AUTOINPUT_UI_WITH_IMGUI_NODE_EDITOR is ON, but imgui-node-editor was not found.\n"
            "Please provide imgui-node-editor by placing it into 'third_party/imgui-node-editor' or 'extern/imgui-node-editor', "
            "or specify -DIMGUI_NODE_EDITOR_DIR=/path/to/imgui-node-editor."
        )
    endif()
endif()
