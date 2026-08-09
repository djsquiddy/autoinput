function(autoinput_get_common_sources output_var)
    set(sources
            ${CMAKE_SOURCE_DIR}/src/pch.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/autoinput.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/autoinput.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/automationController.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/automationController.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/defaults.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/defaults.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/backend.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/backendFactory.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/backendFactory.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/command.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/command.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/environment.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/environment.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/errorCode.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/errorCode.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/arguments.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/arguments.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/waitDelay.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/waitDelay.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/cliHelpFormatter.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/cliHelpFormatter.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/types.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/types.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/mouse.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/keyboard.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/keyInfo.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/platform.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/utils.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/utils.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/logger.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/logger.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/config.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/config.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/configMetadata.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/configMetadata.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/configValidator.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/configValidator.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/runtimeConfig.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/runtimeConfig.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/handlerState.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/settings.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/settings.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/handler.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/sequence.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/sequence.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/sequenceRecorder.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/sequenceRecorder.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/terminal.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/notifications.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/notifications.cpp
            # Services
            ${CMAKE_SOURCE_DIR}/src/autoinput/services/automationRuntimeClient.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/services/automationRuntimeClient.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/services/configService.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/services/configService.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/services/processTransport.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/services/processTransport.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/services/runtimeProtocol.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/services/runtimeProtocol.cpp

            # Cli source
            ${CMAKE_SOURCE_DIR}/src/autoinput/cli/commandBase.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/cli/commandBase.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/cli/cliApplication.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/cli/cliApplication.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/cli/runCommand.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/cli/runCommand.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/cli/recordCommand.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/cli/recordCommand.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/cli/configCommand.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/cli/configCommand.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/cli/appsCommand.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/cli/appsCommand.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/cli/helpCommand.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/cli/helpCommand.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/cli/settingsCommand.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/cli/settingsCommand.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/cli/serveCommand.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/cli/serveCommand.cpp
    )

    if(WIN32)
        list(APPEND sources
                ${CMAKE_SOURCE_DIR}/src/autoinput/win32/autoInput_win32.cpp
                ${CMAKE_SOURCE_DIR}/src/autoinput/win32/internalData_win32.h
                ${CMAKE_SOURCE_DIR}/src/autoinput/win32/notifications_win32.cpp
        )
    elseif(UNIX AND NOT APPLE)
        list(APPEND sources
                ${CMAKE_SOURCE_DIR}/src/autoinput/linux/autoInput_linux.cpp
                ${CMAKE_SOURCE_DIR}/src/autoinput/linux/autoInput_x11.cpp
                ${CMAKE_SOURCE_DIR}/src/autoinput/linux/autoInput_wayland.cpp
                ${CMAKE_SOURCE_DIR}/src/autoinput/linux/internalData_linux.h
                ${CMAKE_SOURCE_DIR}/src/autoinput/linux/notifications_linux.cpp
        )
    endif()

    set(${output_var} ${sources} PARENT_SCOPE)
endfunction()
