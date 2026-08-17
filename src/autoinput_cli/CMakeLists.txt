
add_executable(${PROJECT_NAME}
        ${CMAKE_CURRENT_LIST_DIR}/main.cpp
)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        ${PROJECT_NAME_CORE}
)

configure_autoinput_target(${PROJECT_NAME})
configure_autoinput_platform(${PROJECT_NAME})
