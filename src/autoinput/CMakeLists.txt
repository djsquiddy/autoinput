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

# TODO: add back in
#add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy_directory
#        "${CMAKE_SOURCE_DIR}/configs"
#        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/configs"
#)

