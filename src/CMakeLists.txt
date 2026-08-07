autoinput_get_common_sources(AUTOINPUT_COMMON_SOURCES)

add_library(autoinput_core STATIC
        ${AUTOINPUT_COMMON_SOURCES}
)

target_include_directories(autoinput_core
        PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(autoinput_core
        PUBLIC
        tomlplusplus::tomlplusplus
        Microsoft.GSL::GSL
)

configure_autoinput_target(autoinput_core)
configure_autoinput_platform(autoinput_core)

add_executable(${PROJECT_NAME}
        autoinput_cli/main.cpp
)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        autoinput_core
)

configure_autoinput_target(${PROJECT_NAME})
configure_autoinput_platform(${PROJECT_NAME})

if(AUTOINPUT_BUILD_TRAY)
    set(AUTOINPUT_TRAY_SOURCES
            autoinput_tray/main.cpp
            autoinput_tray/trayApp.cpp
            autoinput_tray/trayApp.h
            autoinput_tray/trayIcon.cpp
            autoinput_tray/trayIcon.h
    )

    if(WIN32)
        list(APPEND AUTOINPUT_TRAY_SOURCES
                autoinput_tray/win32/win32TrayApp.cpp
                autoinput_tray/win32/win32TrayApp.h
                autoinput_tray/win32/win32TrayIcon.cpp
                autoinput_tray/win32/win32TrayIcon.h
                autoinput_tray/win32/resources.rc
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
endif()
