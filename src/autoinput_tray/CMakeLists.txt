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

    add_executable(autoinput_tray
        ${AUTOINPUT_TRAY_SOURCES}
    )

    target_link_libraries(autoinput_tray
        PRIVATE
        autoinput_core
    )

    if(WIN32)
        target_link_libraries(autoinput_tray
            PRIVATE
            gdiplus
            ole32
            shell32
            user32
        )
        set_target_properties(autoinput_tray PROPERTIES
                WIN32_EXECUTABLE TRUE
        )
    elseif(UNIX AND NOT APPLE)
        message(STATUS "Tray support for Linux is currently a stub.")
        # Add Linux-specific tray libraries here when implemented (e.g., appindicator)
    endif()

    configure_autoinput_target(autoinput_tray)
    configure_autoinput_platform(autoinput_tray)
endif(AUTOINPUT_BUILD_TRAY)
