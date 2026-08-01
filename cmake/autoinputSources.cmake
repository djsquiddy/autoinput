function(autoinput_get_common_sources output_var)
    set(sources
            ${CMAKE_SOURCE_DIR}/src/pch.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/autoInput.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/backend.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/arguments.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/arguments.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/cliHelpFormatter.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/cliHelpFormatter.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/types.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/types.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/mouse.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/keyboard.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/keyInfo.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/platform.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/autoInput.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/utils.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/utils.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/logger.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/logger.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/config.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/config.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/handlerState.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/settings.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/settings.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/handler.cpp
    )

    if(WIN32)
        list(APPEND sources
                ${CMAKE_SOURCE_DIR}/src/autoinput/win32/autoInput_win32.cpp
                ${CMAKE_SOURCE_DIR}/src/autoinput/win32/internalData_win32.h
        )
    elseif(UNIX AND NOT APPLE)
        list(APPEND sources
                ${CMAKE_SOURCE_DIR}/src/autoinput/linux/autoInput_linux.cpp
                ${CMAKE_SOURCE_DIR}/src/autoinput/linux/autoInput_x11.cpp
                ${CMAKE_SOURCE_DIR}/src/autoinput/linux/autoInput_wayland.cpp
                ${CMAKE_SOURCE_DIR}/src/autoinput/linux/internalData_linux.h
        )
    endif()

    set(${output_var} ${sources} PARENT_SCOPE)
endfunction()

function(autoinput_get_test_sources output_var)
    set(sources
            ${CMAKE_SOURCE_DIR}/tests/autoinput/keyTest.cpp
            ${CMAKE_SOURCE_DIR}/tests/autoinput/typesTest.cpp
            ${CMAKE_SOURCE_DIR}/tests/autoinput/mouseTest.cpp
            ${CMAKE_SOURCE_DIR}/tests/autoinput/utilsTest.cpp
            ${CMAKE_SOURCE_DIR}/tests/autoinput/argumentsTest.cpp
            ${CMAKE_SOURCE_DIR}/tests/autoinput/configTest.cpp
            ${CMAKE_SOURCE_DIR}/tests/autoinput/positionalArgumentsTest.cpp
            ${CMAKE_SOURCE_DIR}/tests/autoinput/mouseTriggerLogicTest.cpp
            ${CMAKE_SOURCE_DIR}/tests/autoinput/settingsTest.cpp
            ${CMAKE_SOURCE_DIR}/tests/autoinput/handlerTest.cpp
            ${CMAKE_SOURCE_DIR}/tests/autoinput/mousePrioritizationTest.cpp
            ${CMAKE_SOURCE_DIR}/tests/autoinput/mouseTest_win32.cpp
            ${CMAKE_SOURCE_DIR}/tests/autoinput/signalCleanupTest.cpp
            ${CMAKE_SOURCE_DIR}/tests/autoinput/mouseModifierTest.cpp
            ${CMAKE_SOURCE_DIR}/tests/autoinput/userSettingsTest.cpp
            ${CMAKE_SOURCE_DIR}/tests/autoinput/saveConfigTest.cpp
            ${CMAKE_SOURCE_DIR}/tests/autoinput/focusTest.cpp
            ${CMAKE_SOURCE_DIR}/tests/autoinput/configLookupTest.cpp
            ${CMAKE_SOURCE_DIR}/tests/autoinput/testEnvironment.cpp
            ${CMAKE_SOURCE_DIR}/tests/autoinput/testUtils.h
    )

    if(UNIX AND NOT APPLE)
        list(APPEND sources
                ${CMAKE_SOURCE_DIR}/tests/autoinput/backendTest_linux.cpp
                ${CMAKE_SOURCE_DIR}/tests/autoinput/dispatchTest_linux.cpp
        )
    endif()

    set(${output_var} ${sources} PARENT_SCOPE)
endfunction()
