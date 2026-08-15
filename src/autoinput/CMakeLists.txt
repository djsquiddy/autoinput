autoinput_get_common_sources(AUTOINPUT_COMMON_SOURCES)

add_library(autoinput_core STATIC
        ${AUTOINPUT_COMMON_SOURCES}
)

target_include_directories(autoinput_core
        PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${LOCALIZATION_GENERATED_DIR}
        ${LOCALIZATION_GENERATED_DIR}/autoinput_ui/core
)

add_dependencies(autoinput_core generate_localization_ids)

target_link_libraries(autoinput_core
        PUBLIC
        tomlplusplus::tomlplusplus
        Microsoft.GSL::GSL
)

add_dependencies(autoinput_core generate_localization_ids)
configure_autoinput_target(autoinput_core)
configure_autoinput_platform(autoinput_core)

