include(GoogleTest)

autoinput_get_test_sources(AUTOINPUT_TEST_SOURCES)

add_executable(${PROJECT_NAME}_tests
        ${AUTOINPUT_TEST_SOURCES}
)

target_link_libraries(${PROJECT_NAME}_tests
        PRIVATE
        autoinput_core
        GTest::gtest_main
        GTest::gmock
)

configure_autoinput_target(${PROJECT_NAME}_tests)
configure_autoinput_platform(${PROJECT_NAME}_tests)

add_custom_command(TARGET ${PROJECT_NAME}_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/configs"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}_tests>/configs"
)

target_precompile_headers(${PROJECT_NAME}_tests
        PRIVATE
        ${CMAKE_SOURCE_DIR}/src/pch.h
)

gtest_discover_tests(${PROJECT_NAME}_tests)
