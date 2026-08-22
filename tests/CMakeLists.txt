enable_testing()
include(configure_googletest)
include(GoogleTest)

set(AUTOINPUT_TEST_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/keyTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/configMetadataTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/typesTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/mouseTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/utilsTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/argumentsTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/cliCommandTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/cliHelpMetadataTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/serveCommandTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/runtimeProtocolTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/processTransportTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/processTransportIntegrationTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/recorderArgumentsTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/recorderTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/configTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/configValidatorTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/configValidationIntegrationTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/validateConfigCliTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/duplicateConfigCliTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/runtimeConfigTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/positionalArgumentsTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/mouseTriggerLogicTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/settingsTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/handlerTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/mousePrioritizationTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/notificationTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/mouseTestWin32.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/win32/notificationWin32Test.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/signalCleanupTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/mouseModifierTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/userSettingsTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/sequenceTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/saveConfigTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/waitDelayTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/exclusivityTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/focusTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/stopTriggerTestWin32.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/configLookupTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/statusIndicatorTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/fixVerificationTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/multipleCommandNotificationTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/runCommandRefactorTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/configServiceTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/automationControllerTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/localizationTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/controlBindingTest.cpp
        ${CMAKE_SOURCE_DIR}/src/autoinput_ui/core/localization.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/testEnvironment.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/backendTestLinux.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/backendCapabilitiesTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/testUtils.h
)

if(UNIX AND NOT APPLE)
    list(APPEND AUTOINPUT_TEST_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/autoinput/dispatchTestLinux.cpp
    )
endif()

add_executable(${PROJECT_NAME_TEST}
        ${AUTOINPUT_TEST_SOURCES}
)

target_link_libraries(${PROJECT_NAME_TEST}
        PRIVATE
        ${PROJECT_NAME_CORE}
        GTest::gtest_main
        GTest::gmock
)

target_compile_definitions(${PROJECT_NAME_TEST} PRIVATE AUTOINPUT_EXE_PATH="$<TARGET_FILE:autoinput>")

configure_autoinput_target(${PROJECT_NAME_TEST})
configure_autoinput_platform(${PROJECT_NAME_TEST})

gtest_discover_tests(${PROJECT_NAME_TEST})
