/**
 * @file runtimeProtocol.h
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_SERVICE_RUNTIME_PROTOCOL_H
#define INCLUDE_AUTOINPUT_SERVICE_RUNTIME_PROTOCOL_H
#pragma once

#include "autoinput/services/automationRuntimeClient.h"
#include "autoinput/types.h"
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
        bool valid{ false }; 
        std::string error; 
    };

    std::string runtimeStatusToString(RuntimeStatus status);
    RuntimeStatus runtimeStatusFromString(std::string_view value);

    std::string buildRuntimeRequest(std::uint64_t id, std::string_view method);
    std::string buildStartRuntimeRequest(std::uint64_t id, std::string_view configName);
    std::string buildRuntimeResponse(std::uint64_t id, const RuntimeOperationResult& result);

    RuntimeProtocolRequest parseRuntimeRequest(std::string_view jsonLine);
    RuntimeOperationResult parseRuntimeResponse(std::string_view jsonLine);

} // namespace autoinput::services 

#endif // INCLUDE_AUTOINPUT_SERVICE_RUNTIME_PROTOCOL_H
