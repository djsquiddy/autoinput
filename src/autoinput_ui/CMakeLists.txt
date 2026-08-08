if(AUTOINPUT_BUILD_UI)
    include(configure_raylib)
    include(configure_dear_imgui)
    include(configure_rlImGui)

    # FetchContent sets these variables
    FetchContent_GetProperties(ImGui)
    FetchContent_GetProperties(rlImGui)

    set(IMGUI_SOURCES
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    )

    set(RLIMGUI_SOURCES
            ${rlimgui_SOURCE_DIR}/rlImGui.cpp
    )

    set(AUTOINPUT_UI_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/main.cpp
            ${CMAKE_CURRENT_LIST_DIR}/uiApplication.cpp
            ${CMAKE_CURRENT_LIST_DIR}/uiApplication.h
            ${CMAKE_CURRENT_LIST_DIR}/settingsEditorWindow.cpp
            ${CMAKE_CURRENT_LIST_DIR}/settingsEditorWindow.h
            ${CMAKE_CURRENT_LIST_DIR}/configEditorWindow.cpp
            ${CMAKE_CURRENT_LIST_DIR}/configEditorWindow.h
            ${IMGUI_SOURCES}
            ${RLIMGUI_SOURCES}
    )

    add_executable(autoinput_ui
            ${AUTOINPUT_UI_SOURCES}
    )

    target_include_directories(autoinput_ui
            PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/misc/cpp
            ${rlimgui_SOURCE_DIR}
    )

    target_link_libraries(autoinput_ui
            PRIVATE
            autoinput_core
            raylib
    )

    if(WIN32)
        target_link_libraries(autoinput_ui
                PRIVATE
                winmm
                gdi32
        )
        set_target_properties(autoinput_ui PROPERTIES
                WIN32_EXECUTABLE TRUE
        )
    endif()

    configure_autoinput_target(autoinput_ui)
    configure_autoinput_platform(autoinput_ui)
endif(AUTOINPUT_BUILD_UI)
