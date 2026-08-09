/**
 * @file runtimeProtocol.cpp
 * @author djsquiddy
 * @date August 2026
 */

#include "autoinput/services/runtimeProtocol.h"
#include <format>
#include <algorithm>
#include <cctype>

namespace autoinput::services
{
    namespace
    {
        std::string jsonEscape(std::string_view str)
        {
            std::string escaped;
            escaped.reserve(str.size());
            for (char c : str)
            {
                switch (c)
                {
                    case '"':  escaped += "\\\""; break;
                    case '\\': escaped += "\\\\"; break;
                    case '\n': escaped += "\\n"; break;
                    case '\r': escaped += "\\r"; break;
                    case '\t': escaped += "\\t"; break;
                    default:
                        if (static_cast<unsigned char>(c) < 32)
                        {
                            escaped += std::format("\\u{:04x}", static_cast<int>(c));
                        }
                        else
                        {
                            escaped += c;
                        }
                        break;
                }
            }
            return escaped;
        }

        std::string_view trim(std::string_view str)
        {
            auto first = str.find_first_not_of(" \t\r\n");
            if (first == std::string_view::npos) return {};
            auto last = str.find_last_not_of(" \t\r\n");
            return str.substr(first, last - first + 1);
        }

        std::string jsonUnescape(std::string_view str)
        {
            std::string unescaped;
            unescaped.reserve(str.size());
            for (size_t i = 0; i < str.size(); ++i)
            {
                if (str[i] == '\\' && i + 1 < str.size())
                {
                    switch (str[++i])
                    {
                        case '"':  unescaped += '"'; break;
                        case '\\': unescaped += '\\'; break;
                        case 'n':  unescaped += '\n'; break;
                        case 'r':  unescaped += '\r'; break;
                        case 't':  unescaped += '\t'; break;
                        case 'u':
                            if (i + 4 < str.size())
                            {
                                std::string hex(std::string(str.substr(i + 1, 4)));
                                try {
                                    unsigned long val = std::stoul(hex, nullptr, 16);
                                    if (val <= 0xFF) {
                                        unescaped += static_cast<char>(val);
                                        i += 4;
                                    } else {
                                        unescaped += "\\u";
                                        unescaped += hex;
                                        i += 4;
                                    }
                                } catch (...) {
                                    unescaped += 'u';
                                }
                            }
                            else
                            {
                                unescaped += 'u';
                            }
                            break;
                        default:
                            unescaped += str[i];
                            break;
                    }
                }
                else
                {
                    unescaped += str[i];
                }
            }
            return unescaped;
        }

        std::string_view findRawValue(std::string_view json, std::string_view key, bool& isString)
        {
            isString = false;
            std::string searchKey = std::format("\"{}\"", key);
            auto keyPos = json.find(searchKey);
            if (keyPos == std::string_view::npos) return {};

            auto colonPos = json.find(':', keyPos + searchKey.size());
            if (colonPos == std::string_view::npos) return {};

            auto valueStart = json.find_first_not_of(" \t\r\n", colonPos + 1);
            if (valueStart == std::string_view::npos) return {};

            if (json[valueStart] == '"')
            {
                isString = true;
                size_t i = valueStart + 1;
                while (i < json.size())
                {
                    if (json[i] == '\\')
                    {
                        i += 2;
                    }
                    else if (json[i] == '"')
                    {
                        return json.substr(valueStart + 1, i - valueStart - 1);
                    }
                    else
                    {
                        i++;
                    }
                }
                return {};
            }
            else
            {
                auto valueEnd = json.find_first_of(", \t\r\n}", valueStart);
                if (valueEnd == std::string_view::npos) return json.substr(valueStart);
                return json.substr(valueStart, valueEnd - valueStart);
            }
        }
    }

    std::string runtimeStatusToString(RuntimeStatus status)
    {
        switch (status)
        {
            case RuntimeStatus::Stopped:  return "stopped";
            case RuntimeStatus::Starting: return "starting";
            case RuntimeStatus::Running:  return "running";
            case RuntimeStatus::Paused:   return "paused";
            case RuntimeStatus::Error:    return "error";
            default:                      return "unknown";
        }
    }

    RuntimeStatus runtimeStatusFromString(std::string_view value)
    {
        if (value == "stopped")
        {
            return RuntimeStatus::Stopped;
        }
        if (value == "starting")
        {
            return RuntimeStatus::Starting;
        }
        if (value == "running")
        {
            return RuntimeStatus::Running;
        }
        if (value == "paused")
        {
            return RuntimeStatus::Paused;
        }
        if (value == "error")
        {
            return RuntimeStatus::Error;
        }
        return RuntimeStatus::Stopped;
    }

    std::string buildRuntimeRequest(std::uint64_t id, std::string_view method)
    {
        return std::format("{{\"id\":{},\"method\":\"{}\"}}", id, jsonEscape(method));
    }

    std::string buildStartRuntimeRequest(std::uint64_t id, std::string_view configName)
    {
        return std::format("{{\"id\":{},\"method\":\"start\",\"params\":{{\"config\":\"{}\"}}}}", id, jsonEscape(configName));
    }

    std::string buildRuntimeResponse(std::uint64_t id, const RuntimeOperationResult& result)
    {
        return std::format("{{\"id\":{},\"success\":{},\"status\":\"{}\",\"message\":\"{}\"}}",
            id,
            result.success ? "true" : "false",
            runtimeStatusToString(result.status),
            jsonEscape(result.message));
    }

    RuntimeProtocolRequest parseRuntimeRequest(std::string_view jsonLine)
    {
        RuntimeProtocolRequest request;
        std::string_view json = trim(jsonLine);

        if (json.empty() || json.front() != '{' || json.back() != '}')
        {
            request.error = "Invalid JSON object.";
            return request;
        }

        bool isString = false;
        std::string_view idStr = findRawValue(json, "id", isString);
        if (idStr.empty())
        {
            request.error = "Missing request ID.";
            return request;
        }

        if (isString)
        {
            request.error = "Invalid request ID.";
            return request;
        }

        if (!std::ranges::all_of(idStr, [](const char c) { return std::isdigit(static_cast<unsigned char>(c)); }))
        {
            request.error = "Invalid request ID.";
            return request;
        }

        try {
            request.id = std::stoull(std::string(idStr));
        } catch (...) {
            request.error = "Invalid request ID.";
            return request;
        }

        std::string_view methodStr = findRawValue(json, "method", isString);
        if (methodStr.empty())
        {
            request.error = "Missing method.";
            return request;
        }

        if (!isString)
        {
            request.error = "Invalid method.";
            return request;
        }

        request.method = jsonUnescape(methodStr);

        if (request.method == "start")
        {
            auto paramsPos = json.find("\"params\"");
            if (paramsPos != std::string_view::npos)
            {
                std::string_view paramsSub = json.substr(paramsPos);
                std::string_view configStr = findRawValue(paramsSub, "config", isString);
                if (!configStr.empty() && !isString)
                {
                    request.error = "Invalid config for start request.";
                    return request;
                }

                request.config = isString ? jsonUnescape(configStr) : std::string(configStr);
            }

            if (request.config.empty())
            {
                request.error = "Missing config for start request.";
                return request;
            }
        }

        request.valid = true;
        return request;
    }

    RuntimeOperationResult parseRuntimeResponse(std::string_view jsonLine)
    {
        RuntimeOperationResult result;
        result.success = false;
        result.status = RuntimeStatus::Stopped;

        std::string_view json = trim(jsonLine);
        if (json.empty() || json.front() != '{' || json.back() != '}') return result;

        bool isString = false;
        std::string_view successStr = findRawValue(json, "success", isString);
        result.success = (successStr == "true");

        std::string_view statusStr = findRawValue(json, "status", isString);
        result.status = runtimeStatusFromString(isString ? jsonUnescape(statusStr) : statusStr);

        std::string_view messageStr = findRawValue(json, "message", isString);
        result.message = isString ? jsonUnescape(messageStr) : std::string(messageStr);

        return result;
    }

} // namespace autoinput::services
