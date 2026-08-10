/**
 * @file errorCode.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_SUPPORT_ERRORCODE_H
#define INCLUDE_AUTOINPUT_SUPPORT_ERRORCODE_H
#pragma once

#include "autoinput/support/types.h"
#include <cstdint>
#include <cstdlib>
#include <string>
#include <type_traits>
#include <utility>
#include <string_view>

namespace autoinput
{
    /**
     * @brief Codes representing different types of errors in the application.
     */
    enum class ErrorCode : i32
    {
        Success = EXIT_SUCCESS,              /**< Execution finished successfully. */
        HelpRequested = 100,                 /**< User requested help information. */
        InvalidParam = 101,                  /**< An invalid parameter was provided. */
        FailedToInstallHooks = 102,          /**< Could not install system input hooks. */
        UnhandledException = 103,            /**< An unexpected exception occurred. */
        FailedToLoadConfig = 104,            /**< Configuration file could not be loaded. */
        InvalidConfigPath = 105,             /**< The provided configuration path is invalid. */
        // Cli Errors.
        FailedToParseGlobalOptions = 106,    /**< Failed to parse CLI global options. */
        UnknownGlobalOption = 107,           /**< An unknown global option was encountered. */
        UnknownCommandOption = 108,          /**< An unknown command-specific option was encountered. */
        UnknownCommand = 109,                /**< An unknown CLI command was provided. */
        FailedToParseCommandOptions = 110,   /**< Failed to parse CLI command options. */
        UnexpectedArgument = 111,            /**< An unexpected argument was found. */
        CliValidationError = 112,            /**< CLI arguments failed validation. */
        MissingCommandLineArgument = 113,     /**< A required command line argument was missing. */
    };

    /**
     * @brief Gets both the underlying value and string representation of an ErrorCode.
     * @param errorCode The error code.
     * @return A pair containing the integer value and string name.
     */
    std::pair<std::underlying_type_t<ErrorCode>, std::string> errorCodeToStringAndValue(ErrorCode errorCode);

    /**
     * @brief Represents an error with a code and a descriptive message.
     */
    struct ErrorMessage
    {
        ErrorCode code;
        std::string_view message;

        /**
         * @brief Converts the error message to a JSON string.
         * @param indent Indentation level for the JSON string.
         * @return The JSON string.
         */
        [[nodiscard]] std::string toJson(i32 indent = 0) const;
    };

    /**
     * @brief Prints a single error message in JSON format to the console.
     * @param error The error to print.
     */
    void printErrorJson(const ErrorMessage& error);
    
    /**
     * @brief Prints multiple error messages in JSON format to the console.
     * @param errors The vector of errors to print.
     */
    void printErrorJson(const std::vector<ErrorMessage>& errors);
}

#endif // INCLUDE_AUTOINPUT_SUPPORT_ERRORCODE_H
