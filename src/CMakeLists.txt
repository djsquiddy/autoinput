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
    add_executable(autoinput_tray
            autoinput_tray/main.cpp
            autoinput_tray/trayApp.cpp
            autoinput_tray/trayApp.h
    )

    target_link_libraries(autoinput_tray
            PRIVATE
            autoinput_core
    )

    configure_autoinput_target(autoinput_tray)
    configure_autoinput_platform(autoinput_tray)

    if(WIN32)
        set_target_properties(autoinput_tray PROPERTIES
                WIN32_EXECUTABLE TRUE
        )
    endif()
endif()
