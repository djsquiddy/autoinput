if(AUTOINPUT_BUILD_UI)
    include(configure_raylib)
    include(configure_dear_imgui)

    # FetchContent sets these variables
    FetchContent_GetProperties(ImGui)

    set(IMGUI_SOURCES
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    )

    set(AUTOINPUT_UI_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/main.cpp
            ${CMAKE_CURRENT_LIST_DIR}/ui.h
            ${CMAKE_CURRENT_LIST_DIR}/imgui_impl_raylib.cpp
            ${CMAKE_CURRENT_LIST_DIR}/imgui_impl_raylib.h

            # App
            ${CMAKE_CURRENT_LIST_DIR}/app/uiApplication.cpp
            ${CMAKE_CURRENT_LIST_DIR}/app/uiApplication.h
            # Core
            ${CMAKE_CURRENT_LIST_DIR}/core/uiWindow.cpp
            ${CMAKE_CURRENT_LIST_DIR}/core/uiWindow.h
            ${CMAKE_CURRENT_LIST_DIR}/core/windowManager.cpp
            ${CMAKE_CURRENT_LIST_DIR}/core/windowManager.h
            ${CMAKE_CURRENT_LIST_DIR}/core/imguiScope.h
            # Widgets
            ${CMAKE_CURRENT_LIST_DIR}/widgets/basicWidgets.cpp
            ${CMAKE_CURRENT_LIST_DIR}/widgets/basicWidgets.h
            ${CMAKE_CURRENT_LIST_DIR}/widgets/formWidgets.cpp
            ${CMAKE_CURRENT_LIST_DIR}/widgets/formWidgets.h
            ${CMAKE_CURRENT_LIST_DIR}/widgets/modalWidgets.cpp
            ${CMAKE_CURRENT_LIST_DIR}/widgets/modalWidgets.h
            # Editors
            ${CMAKE_CURRENT_LIST_DIR}/editors/globalSettingsEditor.cpp
            ${CMAKE_CURRENT_LIST_DIR}/editors/globalSettingsEditor.h
            ${CMAKE_CURRENT_LIST_DIR}/editors/commandEditor.cpp
            ${CMAKE_CURRENT_LIST_DIR}/editors/commandEditor.h
            ${CMAKE_CURRENT_LIST_DIR}/editors/sequenceViewer.cpp
            ${CMAKE_CURRENT_LIST_DIR}/editors/sequenceViewer.h
            # Windows
            ${CMAKE_CURRENT_LIST_DIR}/windows/mainWindow.cpp
            ${CMAKE_CURRENT_LIST_DIR}/windows/mainWindow.h
            ${CMAKE_CURRENT_LIST_DIR}/windows/settingsEditorWindow.cpp
            ${CMAKE_CURRENT_LIST_DIR}/windows/settingsEditorWindow.h
            ${CMAKE_CURRENT_LIST_DIR}/windows/configEditorWindow.cpp
            ${CMAKE_CURRENT_LIST_DIR}/windows/configEditorWindow.h
            ${CMAKE_CURRENT_LIST_DIR}/windows/aboutWindow.cpp
            ${CMAKE_CURRENT_LIST_DIR}/windows/aboutWindow.h

            # External sources
            ${IMGUI_SOURCES}
    )

    add_executable(autoinput_ui
            ${AUTOINPUT_UI_SOURCES}
    )

    target_include_directories(autoinput_ui
            PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/misc/cpp
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
