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
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/graphModelTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/graphValidatorTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/sequenceGraphAdapterTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/sequenceGraphCompilerTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/nodeEditorBackendTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/fallbackGraphViewerTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/sequenceGraphEditorTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/configGraphAdapterTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/configGraphViewerTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/configDiagnosticsTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/customCanvasBackendTest.cpp
        ${CMAKE_CURRENT_LIST_DIR}/autoinput/graphLayoutMetadataTest.cpp
        ${CMAKE_SOURCE_DIR}/src/autoinput_ui/core/localization.cpp
        ${CMAKE_SOURCE_DIR}/src/autoinput_ui/graph/graphModel.cpp
        ${CMAKE_SOURCE_DIR}/src/autoinput_ui/graph/graphValidator.cpp
        ${CMAKE_SOURCE_DIR}/src/autoinput_ui/graph/sequenceGraphAdapter.cpp
        ${CMAKE_SOURCE_DIR}/src/autoinput_ui/graph/sequenceGraphCompiler.cpp
        ${CMAKE_SOURCE_DIR}/src/autoinput_ui/graph/configGraphAdapter.cpp
        ${CMAKE_SOURCE_DIR}/src/autoinput_ui/graph/configDiagnostics.cpp
        ${CMAKE_SOURCE_DIR}/src/autoinput_ui/graph/customCanvasBackend.cpp
        ${CMAKE_SOURCE_DIR}/src/autoinput_ui/graph/graphLayoutMetadata.cpp
        ${CMAKE_SOURCE_DIR}/src/autoinput_ui/graph/nodeEditorBackend.cpp
        ${CMAKE_SOURCE_DIR}/src/autoinput_ui/graph/fallbackGraphViewer.cpp
        ${CMAKE_SOURCE_DIR}/src/autoinput_ui/editors/sequenceGraphEditor.cpp
        ${CMAKE_SOURCE_DIR}/src/autoinput_ui/editors/configGraphViewer.cpp
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

if(AUTOINPUT_UI_WITH_IMNODES AND IMNODES_FOUND)
    list(APPEND AUTOINPUT_TEST_SOURCES
            ${CMAKE_SOURCE_DIR}/src/autoinput_ui/graph/imnodesBackend.cpp
            ${IMNODES_SOURCES}
    )
endif()

if(AUTOINPUT_UI_WITH_IMGUI_NODE_EDITOR AND IMGUI_NODE_EDITOR_FOUND)
    list(APPEND AUTOINPUT_TEST_SOURCES
            ${CMAKE_SOURCE_DIR}/src/autoinput_ui/graph/imguiNodeEditorBackend.cpp
            ${IMGUI_NODE_EDITOR_SOURCES}
    )
endif()

add_executable(${PROJECT_NAME_TEST}
        ${AUTOINPUT_TEST_SOURCES}
)

if(AUTOINPUT_UI_WITH_IMNODES AND IMNODES_FOUND)
    target_compile_definitions(${PROJECT_NAME_TEST} PRIVATE AUTOINPUT_HAS_IMNODES=1)
    target_include_directories(${PROJECT_NAME_TEST} PRIVATE ${IMNODES_INCLUDE_DIR})
endif()

if(AUTOINPUT_UI_WITH_IMGUI_NODE_EDITOR AND IMGUI_NODE_EDITOR_FOUND)
    target_compile_definitions(${PROJECT_NAME_TEST} PRIVATE AUTOINPUT_HAS_IMGUI_NODE_EDITOR=1)
    target_include_directories(${PROJECT_NAME_TEST} PRIVATE ${IMGUI_NODE_EDITOR_INCLUDE_DIR})
endif()

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

find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
    add_test(
        NAME autoinput_python_tests
        COMMAND "${Python3_EXECUTABLE}" -m pytest "${CMAKE_CURRENT_LIST_DIR}/scripts"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
    set_tests_properties(autoinput_python_tests PROPERTIES
        ENVIRONMENT "PYTHONPATH=${SCRIPTS_DIR}"
    )

    add_custom_target(python_tests
        COMMAND ${CMAKE_COMMAND} -E env "PYTHONPATH=${SCRIPTS_DIR}" "${Python3_EXECUTABLE}" -m pytest "${CMAKE_CURRENT_LIST_DIR}/scripts"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Running Python unit tests with pytest"
        VERBATIM
    )
endif()
