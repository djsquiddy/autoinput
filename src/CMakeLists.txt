autoinput_get_common_sources(AUTOINPUT_COMMON_SOURCES)

add_executable(${PROJECT_NAME}
        autoinput/main.cpp
        ${AUTOINPUT_COMMON_SOURCES}
)

target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        tomlplusplus::tomlplusplus
        Microsoft.GSL::GSL
)

configure_autoinput_target(${PROJECT_NAME})
configure_autoinput_platform(${PROJECT_NAME})

target_precompile_headers(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_SOURCE_DIR}/src/pch.h
)
