autoinput_get_common_sources(AUTOINPUT_COMMON_SOURCES)
autoinput_group_sources_by_directory("${CMAKE_SOURCE_DIR}/src/autoinput" ${AUTOINPUT_COMMON_SOURCES})

add_library(${PROJECT_NAME_CORE} STATIC
        ${AUTOINPUT_COMMON_SOURCES}
)

target_include_directories(${PROJECT_NAME_CORE}
        PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${CODE_GENERATED_DIR}
)

add_dependencies(${PROJECT_NAME_CORE} generate_localization_ids)

target_link_libraries(${PROJECT_NAME_CORE}
        PUBLIC
        tomlplusplus::tomlplusplus
        Microsoft.GSL::GSL
)

add_dependencies(${PROJECT_NAME_CORE} generate_localization_ids)
configure_autoinput_target(${PROJECT_NAME_CORE})
configure_autoinput_platform(${PROJECT_NAME_CORE})
