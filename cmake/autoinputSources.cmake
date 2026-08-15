function(autoinput_get_common_sources output_var)
    set(sources
            ${CMAKE_SOURCE_DIR}/src/pch.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/app/autoinput.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/app/autoinput.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/app/automationController.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/app/automationController.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/config/defaults.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/config/defaults.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/platform/backend.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/platform/backendFactory.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/platform/backendFactory.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/input/command.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/input/command.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/platform/environment.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/platform/environment.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/support/errorCode.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/support/errorCode.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/cli/arguments.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/cli/arguments.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/input/waitDelay.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/input/waitDelay.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/cli/cliHelpFormatter.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/cli/cliHelpFormatter.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/support/types.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/support/types.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/input/mouse.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/input/keyboard.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/input/keyInfo.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/platform/platform.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/support/utils.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/support/utils.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/support/logger.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/support/logger.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/config/config.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/config/config.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/config/configMetadata.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/config/configMetadata.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/config/configValidator.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/config/configValidator.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/config/runtimeConfig.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/config/runtimeConfig.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/app/handlerState.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/config/settings.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/config/settings.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/app/handler.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/input/sequence.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/input/sequence.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/input/sequenceRecorder.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/input/sequenceRecorder.cpp
            ${CMAKE_SOURCE_DIR}/src/autoinput/platform/terminal.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/platform/notifications.h
            ${CMAKE_SOURCE_DIR}/src/autoinput/platform/notifications.cpp
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
                ${CMAKE_SOURCE_DIR}/src/autoinput/platform/win32/autoInputWin32.cpp
                ${CMAKE_SOURCE_DIR}/src/autoinput/platform/win32/internalDataWin32.h
                ${CMAKE_SOURCE_DIR}/src/autoinput/platform/win32/notificationsWin32.cpp
                # Services
                ${CMAKE_SOURCE_DIR}/src/autoinput/services/platform/processTransportWin32.cpp

        )
    elseif(UNIX AND NOT APPLE)
        list(APPEND sources
                ${CMAKE_SOURCE_DIR}/src/autoinput/platform/linux/autoInputLinux.cpp
                ${CMAKE_SOURCE_DIR}/src/autoinput/platform/linux/autoInputX11.cpp
                ${CMAKE_SOURCE_DIR}/src/autoinput/platform/linux/autoInputWayland.cpp
                ${CMAKE_SOURCE_DIR}/src/autoinput/platform/linux/internalDataLinux.h
                ${CMAKE_SOURCE_DIR}/src/autoinput/platform/linux/notificationsLinux.cpp
        )
    endif()

    set(${output_var} ${sources} PARENT_SCOPE)
endfunction()
