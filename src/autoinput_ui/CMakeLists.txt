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
            ${CMAKE_CURRENT_LIST_DIR}/core/uibackend.cpp
            ${CMAKE_CURRENT_LIST_DIR}/core/uibackend.h

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
            ${CMAKE_CURRENT_LIST_DIR}/windows/runtimeWindow.cpp
            ${CMAKE_CURRENT_LIST_DIR}/windows/runtimeWindow.h
            ${CMAKE_CURRENT_LIST_DIR}/windows/runtimeDashboardWindow.cpp
            ${CMAKE_CURRENT_LIST_DIR}/windows/runtimeDashboardWindow.h
            ${CMAKE_CURRENT_LIST_DIR}/windows/commandRunnerWindow.cpp
            ${CMAKE_CURRENT_LIST_DIR}/windows/commandRunnerWindow.h
            ${CMAKE_CURRENT_LIST_DIR}/windows/logViewerWindow.cpp
            ${CMAKE_CURRENT_LIST_DIR}/windows/logViewerWindow.h
            ${CMAKE_CURRENT_LIST_DIR}/windows/backendDiagnosticsWindow.cpp
            ${CMAKE_CURRENT_LIST_DIR}/windows/backendDiagnosticsWindow.h
            ${CMAKE_CURRENT_LIST_DIR}/windows/sequenceRecorderWindow.cpp
            ${CMAKE_CURRENT_LIST_DIR}/windows/sequenceRecorderWindow.h
            ${CMAKE_CURRENT_LIST_DIR}/windows/sequenceEditorWindow.cpp
            ${CMAKE_CURRENT_LIST_DIR}/windows/sequenceEditorWindow.h
            ${CMAKE_CURRENT_LIST_DIR}/windows/configManagerWindow.cpp
            ${CMAKE_CURRENT_LIST_DIR}/windows/configManagerWindow.h
            ${CMAKE_CURRENT_LIST_DIR}/windows/hotkeyManagerWindow.cpp
            ${CMAKE_CURRENT_LIST_DIR}/windows/hotkeyManagerWindow.h
            ${CMAKE_CURRENT_LIST_DIR}/windows/applicationPickerWindow.cpp
            ${CMAKE_CURRENT_LIST_DIR}/windows/applicationPickerWindow.h
            ${CMAKE_CURRENT_LIST_DIR}/windows/validationReportWindow.cpp
            ${CMAKE_CURRENT_LIST_DIR}/windows/validationReportWindow.h
            ${CMAKE_CURRENT_LIST_DIR}/windows/setupWizardWindow.cpp
            ${CMAKE_CURRENT_LIST_DIR}/windows/setupWizardWindow.h
            ${CMAKE_CURRENT_LIST_DIR}/windows/notificationTesterWindow.cpp
            ${CMAKE_CURRENT_LIST_DIR}/windows/notificationTesterWindow.h
            ${CMAKE_CURRENT_LIST_DIR}/windows/importExportWindow.cpp
            ${CMAKE_CURRENT_LIST_DIR}/windows/importExportWindow.h
            ${CMAKE_CURRENT_LIST_DIR}/windows/commandPaletteWindow.cpp
            ${CMAKE_CURRENT_LIST_DIR}/windows/commandPaletteWindow.h

            ${CMAKE_CURRENT_LIST_DIR}/windows/backupRestoreWindow.cpp
            ${CMAKE_CURRENT_LIST_DIR}/windows/backupRestoreWindow.h

            # External sources
            ${IMGUI_SOURCES}
    )

    add_executable(${PROJECT_NAME_UI}
            ${AUTOINPUT_UI_SOURCES}
    )

    target_include_directories(${PROJECT_NAME_UI}
            PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/misc/cpp
    )

    target_link_libraries(${PROJECT_NAME_UI}
            PRIVATE
            autoinput_core
            raylib
    )

    if(WIN32)
        target_link_libraries(${PROJECT_NAME_UI}
                PRIVATE
                winmm
                gdi32
        )
        set_target_properties(${PROJECT_NAME_UI} PROPERTIES
                WIN32_EXECUTABLE TRUE
        )
    endif()

    configure_autoinput_target(${PROJECT_NAME_UI})
    configure_autoinput_platform(${PROJECT_NAME_UI})
endif(AUTOINPUT_BUILD_UI)
