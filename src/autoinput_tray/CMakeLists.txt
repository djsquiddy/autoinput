if(AUTOINPUT_BUILD_TRAY)
    set(AUTOINPUT_TRAY_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/main.cpp
            ${CMAKE_CURRENT_LIST_DIR}/trayApp.cpp
            ${CMAKE_CURRENT_LIST_DIR}/trayApp.h
            ${CMAKE_CURRENT_LIST_DIR}/trayIcon.cpp
            ${CMAKE_CURRENT_LIST_DIR}/trayIcon.h
    )

    if(WIN32)
        list(APPEND AUTOINPUT_TRAY_SOURCES
                ${CMAKE_CURRENT_LIST_DIR}/win32/win32TrayApp.cpp
                ${CMAKE_CURRENT_LIST_DIR}/win32/win32TrayApp.h
                ${CMAKE_CURRENT_LIST_DIR}/win32/win32TrayIcon.cpp
                ${CMAKE_CURRENT_LIST_DIR}/win32/win32TrayIcon.h
                ${CMAKE_CURRENT_LIST_DIR}/win32/resources.rc
        )
    endif()

    autoinput_group_sources_by_directory("${CMAKE_CURRENT_LIST_DIR}" ${AUTOINPUT_TRAY_SOURCES})
    add_executable(${PROJECT_NAME_TRAY} ${AUTOINPUT_TRAY_SOURCES})

    target_link_libraries(${PROJECT_NAME_TRAY} PRIVATE ${PROJECT_NAME_CORE})

    if(WIN32)
        target_link_libraries(${PROJECT_NAME_TRAY}
            PRIVATE
            gdiplus
            ole32
            shell32
            user32
        )
        set_target_properties(${PROJECT_NAME_TRAY} PROPERTIES
                WIN32_EXECUTABLE TRUE
        )
    elseif(UNIX AND NOT APPLE)
        message(STATUS "Tray support for Linux is currently a stub.")
        # Add Linux-specific tray libraries here when implemented (e.g., appindicator)
    endif()

    configure_autoinput_target(${PROJECT_NAME_TRAY})
    configure_autoinput_platform(${PROJECT_NAME_TRAY})
endif(AUTOINPUT_BUILD_TRAY)
