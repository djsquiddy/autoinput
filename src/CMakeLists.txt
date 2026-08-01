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

target_precompile_headers(autoinput_core
        PRIVATE
        ${CMAKE_SOURCE_DIR}/src/pch.h
)

add_executable(${PROJECT_NAME}
        autoinput_cli/main.cpp
)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        autoinput_core
)

configure_autoinput_target(${PROJECT_NAME})
configure_autoinput_platform(${PROJECT_NAME})

target_precompile_headers(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_SOURCE_DIR}/src/pch.h
)
