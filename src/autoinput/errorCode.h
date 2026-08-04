/**
 * @file errorCode.h
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_ERROR_CODE_H
#define INCLUDE_AUTOINPUT_ERROR_CODE_H
#pragma once

#include "autoinput/types.h"
#include <cstdint>
#include <cstdlib>
#include <string>
#include <type_traits>
#include <utility>
#include <string_view>

namespace autoinput
{
    enum class ErrorCode : int32_t
    {
        Success = EXIT_SUCCESS,
        InvalidParam = 101,
        FailedToInstallHooks = 102,
        UnhandledException = 103,
        FailedToLoadConfig = 104,
        InvalidConfigPath = 105,
        // Cli Errors.
        FailedToParseGlobalOptions = 106,
        UnknownCommand = 107,
        FailedToParseCommandOptions = 108,
        UnexpectedArgument = 109,
        CliValidationError = 110,
    };

    std::pair<std::underlying_type_t<ErrorCode>, std::string> errorCodeToStringAndValue(ErrorCode errorCode);

    struct ErrorMessage
    {
        ErrorCode code;
        std::string_view message;

        [[nodiscard]] std::string toJson(i32 indent = 0) const;
    };

    void printErrorJson(const ErrorMessage& error);
    void printErrorJson(const std::vector<ErrorMessage>& errors);
}

#endif // INCLUDE_AUTOINPUT_ERROR_CODE_H
