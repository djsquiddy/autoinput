/**
 * @file runtimeProtocol.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_SERVICES_RUNTIMEPROTOCOL_H
#define INCLUDE_AUTOINPUT_SERVICES_RUNTIMEPROTOCOL_H
#pragma once

#include "autoinput/services/automationRuntimeClient.h"
#include "autoinput/support/types.h"
#include <string>
#include <string_view>
#include <cstdint>

namespace autoinput::services 
{ 
    struct RuntimeProtocolRequest 
    { 
        std::uint64_t id{ 0 }; 
        std::string method; 
        std::string config; 
        std::string command;
        std::string title;
        std::string body;
        NotificationSeverity severity{ NotificationSeverity::Info };
        std::optional<StatusNotificationMode> notificationMode;

        bool recordMouseMoves{ false };
        bool recordMouseClicks{ true };
        bool recordKeyboardEvents{ true };
        bool recordDelays{ true };
        std::string recordName;
        std::string recordStartKey;
        std::string recordEndKey;
        std::string recordPlayStartKey;
        std::string recordMouseSample;

        bool valid{ false }; 
        std::string error; 
    };

    std::string runtimeStatusToString(RuntimeStatus status);
    RuntimeStatus runtimeStatusFromString(std::string_view value);

    std::string buildRuntimeRequest(std::uint64_t id, std::string_view method);
    std::string buildStartRuntimeRequest(std::uint64_t id, std::string_view configName);
    std::string buildRunCommandRequest(std::uint64_t id, std::string_view configName, std::string_view commandName);
    std::string buildTestNotificationRequest(std::uint64_t id, std::string_view title, std::string_view body, NotificationSeverity severity = NotificationSeverity::Info, std::optional<StatusNotificationMode> mode = std::nullopt);
    std::string buildStartRecordingRequest(std::uint64_t id, const SequenceConfig& config);
    std::string buildGetRecordedSequenceRequest(std::uint64_t id);
    std::string buildRuntimeResponse(std::uint64_t id, const RuntimeOperationResult& result);

    RuntimeProtocolRequest parseRuntimeRequest(std::string_view jsonLine);
    RuntimeOperationResult parseRuntimeResponse(std::string_view jsonLine);

} // namespace autoinput::services 

#endif // INCLUDE_AUTOINPUT_SERVICES_RUNTIMEPROTOCOL_H
