/**
 * @file errorCode.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/errorCode.h"
#include "autoinput/logger.h"
#include "autoinput/utils.h"


namespace autoinput
{
    namespace
    {
        using errorCode_t = std::underlying_type_t<ErrorCode>;
    }

    std::pair<std::underlying_type_t<ErrorCode>, std::string> errorCodeToStringAndValue(
        const ErrorCode errorCode)
    {
        std::string name;
        switch (errorCode)
        {
        case ErrorCode::Success:
            name = "Success";
            break;
        case ErrorCode::InvalidParam:
            name = "InvalidParam";
            break;
        case ErrorCode::FailedToInstallHooks:
            name = "FailedToInstallHooks";
            break;
        case ErrorCode::UnhandledException:
            name = "UnhandledException";
            break;
        case ErrorCode::FailedToLoadConfig:
            name = "FailedToLoadConfig";
            break;
        case ErrorCode::InvalidConfigPath:
            name = "InvalidConfigPath";
            break;
        case ErrorCode::FailedToParseGlobalOptions:
            name = "FailedToParseGlobalOptions";
            break;
        case ErrorCode::UnknownCommand:
            name = "UnknownCommand";
            break;
        case ErrorCode::FailedToParseCommandOptions:
            name = "FailedToParseCommandOptions";
            break;
        case ErrorCode::UnexpectedArgument:
            name = "UnexpectedArgument";
            break;
        case ErrorCode::CliValidationError:
            name = "CliValidationError";
            break;
        case ErrorCode::MissingCommandLineArgument:
            name = "MissingCommandLineArgument";
            break;
        case ErrorCode::UnknownCommandOption:
            name = "UnknownCommandOption";
            break;
        default:
            AUTOINPUT_ASSERT(false, "Unknown error code: {}", static_cast<errorCode_t>(errorCode));
        }

        return std::make_pair(static_cast<errorCode_t>(errorCode), name);
    }

    std::string ErrorMessage::toJson(const i32 indent) const
    {
        auto [errorCodeValue, errorName] = errorCodeToStringAndValue(code);
        std::string originalPrefix = std::string(std::max(indent - 1, 0), ' ');
        std::string prefix = std::string(indent, ' ');
        return std::format("\n{}{{\n"
               "{}\"errorCode\": {},\n"
               "{}\"errorName\": \"{}\",\n"
               "{}\"message\": \"{}\"\n"
               "{}}}",
               originalPrefix, prefix, errorCodeValue,
               prefix, errorName,
               prefix, message, originalPrefix);
    }

    void printErrorJson(const ErrorMessage& error)
    {
        Logger::print("{\n");
        Logger::print("  \"errors\": [");
        Logger::print(error.toJson(3));
        Logger::print("\n  ]\n}\n");
    }

    void printErrorJson(const std::vector<ErrorMessage>& errors)
    {
        Logger::print("{\n");
        Logger::print("  \"errors\": [");
        const auto lastIndex = errors.size() - 1;
        for (size_t i = 0; i < errors.size(); ++i)
        {
            Logger::print(errors[i].toJson(/*indent=*/3));
            if (i < lastIndex)
            {
                Logger::print(",");
            }
        }
        if (!errors.empty())
        {
            Logger::print("\n  ");
        }
        Logger::print("]\n}\n");
    }
}
