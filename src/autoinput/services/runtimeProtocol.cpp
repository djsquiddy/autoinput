/**
 * @file runtimeProtocol.cpp
 * @author djsquiddy
 * @date August 2026
 */

#include "autoinput/services/runtimeProtocol.h"
#include <format>
#include <algorithm>
#include <cctype>
#include <charconv>

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

    std::string buildRunCommandRequest(std::uint64_t id, std::string_view configName, std::string_view commandName)
    {
        return std::format("{{\"id\":{},\"method\":\"run_command\",\"params\":{{\"config\":\"{}\",\"command\":\"{}\"}}}}", 
            id, jsonEscape(configName), jsonEscape(commandName));
    }

    std::string buildTestNotificationRequest(std::uint64_t id, std::string_view title, std::string_view body)
    {
        return std::format("{{\"id\":{},\"method\":\"test_notification\",\"params\":{{\"title\":\"{}\",\"body\":\"{}\"}}}}",
            id, jsonEscape(title), jsonEscape(body));
    }

    std::string buildStartRecordingRequest(std::uint64_t id, const SequenceConfig& config)
    {
        return std::format("{{\"id\":{},\"method\":\"start_recording\",\"params\":{{\"name\":\"{}\",\"recordMouseMoves\":{},\"recordMouseClicks\":{},\"recordKeyboardEvents\":{},\"recordDelays\":{},\"startKey\":\"{}\",\"endKey\":\"{}\",\"playStartKey\":\"{}\",\"mouseSampleDelay\":\"{}\"}}}}",
            id, jsonEscape(config.name), config.recordMouseMoves ? "true" : "false", config.recordMouseClicks ? "true" : "false", config.recordKeyboardEvents ? "true" : "false", config.recordDelays ? "true" : "false", jsonEscape(config.startKey), jsonEscape(config.endKey), jsonEscape(config.playStartKey), jsonEscape(config.mouseSampleDelay));
    }

    std::string buildGetRecordedSequenceRequest(std::uint64_t id)
    {
        return std::format("{{\"id\":{},\"method\":\"get_recorded_sequence\"}}", id);
    }

    std::string buildRuntimeResponse(std::uint64_t id, const RuntimeOperationResult& result)
    {
        std::string extra;
        if (!result.backendName.empty())
        {
            extra += std::format(",\"backend_name\":\"{}\"", jsonEscape(result.backendName));
            extra += std::format(",\"capabilities\":{{\"keyboardHooks\":{},\"mouseHooks\":{},\"focusDetection\":{},\"listApplications\":{},\"syntheticKeyboardInput\":{},\"syntheticMouseInput\":{},\"absoluteMouseMovement\":{},\"getCursorPosition\":{}}}",
                result.capabilities.keyboardHooks ? "true" : "false",
                result.capabilities.mouseHooks ? "true" : "false",
                result.capabilities.focusDetection ? "true" : "false",
                result.capabilities.listApplications ? "true" : "false",
                result.capabilities.syntheticKeyboardInput ? "true" : "false",
                result.capabilities.syntheticMouseInput ? "true" : "false",
                result.capabilities.absoluteMouseMovement ? "true" : "false",
                result.capabilities.getCursorPosition ? "true" : "false");
        }
        
        extra += std::format(",\"recording\":{},\"recording_paused\":{},\"recorded_event_count\":{}",
            result.recording ? "true" : "false",
            result.recordingPaused ? "true" : "false",
            result.recordedEventCount);

        if (result.sequence)
        {
            std::string seqJson = std::format(",\"sequence\":{{\"name\":\"{}\",\"start\":\"{}\",\"repeat\":{},\"events\":[",
                jsonEscape(result.sequence->name), jsonEscape(result.sequence->start), result.sequence->repeat ? "true" : "false");
            
            for (size_t i = 0; i < result.sequence->events.size(); ++i)
            {
                const auto& event = result.sequence->events[i];
                seqJson += std::format("{{\"type\":\"{}\",\"delay\":\"{}\"",
                    recordedEventTypeToString(event.type), jsonEscape(event.delay));
                
                if (event.key) seqJson += std::format(",\"key\":\"{}\"", jsonEscape(*event.key));
                if (event.button) seqJson += std::format(",\"button\":\"{}\"", jsonEscape(*event.button));
                if (event.x) seqJson += std::format(",\"x\":{}", *event.x);
                if (event.y) seqJson += std::format(",\"y\":{}", *event.y);
                
                seqJson += "}";
                if (i < result.sequence->events.size() - 1) seqJson += ",";
            }
            seqJson += "]}}";
            extra += seqJson;
        }

        auto appWindowInfoToJson = [](const AppWindowInfo& info) {
            return std::format(
                "{{\"process_name\":\"{}\",\"window_title\":\"{}\",\"pid\":{},\"executable_path\":\"{}\",\"backend_id\":\"{}\"}}",
                jsonEscape(info.processName),
                jsonEscape(info.windowTitle),
                info.pid,
                jsonEscape(info.executablePath),
                jsonEscape(info.backendId)
            );
        };

        if (!result.windows.empty())
        {
            std::string windowsJson = ",\"windows\":[";
            for (size_t i = 0; i < result.windows.size(); ++i)
            {
                windowsJson += appWindowInfoToJson(result.windows[i]);
                if (i < result.windows.size() - 1) windowsJson += ",";
            }
            windowsJson += "]";
            extra += windowsJson;
        }

        if (result.foregroundWindow)
        {
            extra += std::format(",\"foreground_window\":{}", appWindowInfoToJson(*result.foregroundWindow));
        }

        return std::format("{{\"id\":{},\"success\":{},\"status\":\"{}\",\"message\":\"{}\"{}}}",
            id,
            result.success ? "true" : "false",
            runtimeStatusToString(result.status),
            jsonEscape(result.message),
            extra);
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
        else if (request.method == "run_command")
        {
            auto paramsPos = json.find("\"params\"");
            if (paramsPos != std::string_view::npos)
            {
                std::string_view paramsSub = json.substr(paramsPos);
                
                std::string_view configStr = findRawValue(paramsSub, "config", isString);
                if (!configStr.empty())
                {
                    request.config = isString ? jsonUnescape(configStr) : std::string(configStr);
                }

                std::string_view commandStr = findRawValue(paramsSub, "command", isString);
                if (!commandStr.empty())
                {
                    request.command = isString ? jsonUnescape(commandStr) : std::string(commandStr);
                }
            }

            if (request.command.empty())
            {
                request.error = "Missing command for run_command request.";
                return request;
            }
        }
        else if (request.method == "test_notification")
        {
            auto paramsPos = json.find("\"params\"");
            if (paramsPos != std::string_view::npos)
            {
                std::string_view paramsSub = json.substr(paramsPos);
                
                std::string_view titleStr = findRawValue(paramsSub, "title", isString);
                if (!titleStr.empty())
                {
                    request.title = isString ? jsonUnescape(titleStr) : std::string(titleStr);
                }

                std::string_view bodyStr = findRawValue(paramsSub, "body", isString);
                if (!bodyStr.empty())
                {
                    request.body = isString ? jsonUnescape(bodyStr) : std::string(bodyStr);
                }
            }
        }
        else if (request.method == "start_recording")
        {
            auto paramsPos = json.find("\"params\"");
            if (paramsPos != std::string_view::npos)
            {
                std::string_view paramsSub = json.substr(paramsPos);
                
                std::string_view nameStr = findRawValue(paramsSub, "name", isString);
                if (!nameStr.empty())
                    request.recordName = isString ? jsonUnescape(nameStr) : std::string(nameStr);

                std::string_view recordMouseMovesStr = findRawValue(paramsSub, "recordMouseMoves", isString);
                request.recordMouseMoves = (recordMouseMovesStr == "true");

                std::string_view recordMouseClicksStr = findRawValue(paramsSub, "recordMouseClicks", isString);
                if (!recordMouseClicksStr.empty()) request.recordMouseClicks = (recordMouseClicksStr == "true");

                std::string_view recordKeyboardEventsStr = findRawValue(paramsSub, "recordKeyboardEvents", isString);
                if (!recordKeyboardEventsStr.empty()) request.recordKeyboardEvents = (recordKeyboardEventsStr == "true");

                std::string_view recordDelaysStr = findRawValue(paramsSub, "recordDelays", isString);
                if (!recordDelaysStr.empty()) request.recordDelays = (recordDelaysStr == "true");

                std::string_view startKeyStr = findRawValue(paramsSub, "startKey", isString);
                if (!startKeyStr.empty())
                    request.recordStartKey = isString ? jsonUnescape(startKeyStr) : std::string(startKeyStr);

                std::string_view endKeyStr = findRawValue(paramsSub, "endKey", isString);
                if (!endKeyStr.empty())
                    request.recordEndKey = isString ? jsonUnescape(endKeyStr) : std::string(endKeyStr);

                std::string_view playStartKeyStr = findRawValue(paramsSub, "playStartKey", isString);
                if (!playStartKeyStr.empty())
                    request.recordPlayStartKey = isString ? jsonUnescape(playStartKeyStr) : std::string(playStartKeyStr);

                std::string_view mouseSampleDelayStr = findRawValue(paramsSub, "mouseSampleDelay", isString);
                if (!mouseSampleDelayStr.empty())
                    request.recordMouseSample = isString ? jsonUnescape(mouseSampleDelayStr) : std::string(mouseSampleDelayStr);
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

        std::string_view backendNameStr = findRawValue(json, "backend_name", isString);
        if (!backendNameStr.empty())
        {
            result.backendName = isString ? jsonUnescape(backendNameStr) : std::string(backendNameStr);
        }

        auto capsPos = json.find("\"capabilities\"");
        if (capsPos != std::string_view::npos)
        {
            std::string_view capsSub = json.substr(capsPos);
            result.capabilities.keyboardHooks = findRawValue(capsSub, "keyboardHooks", isString) == "true";
            result.capabilities.mouseHooks = findRawValue(capsSub, "mouseHooks", isString) == "true";
            result.capabilities.focusDetection = findRawValue(capsSub, "focusDetection", isString) == "true";
            result.capabilities.listApplications = findRawValue(capsSub, "listApplications", isString) == "true";
            result.capabilities.syntheticKeyboardInput = findRawValue(capsSub, "syntheticKeyboardInput", isString) == "true";
            result.capabilities.syntheticMouseInput = findRawValue(capsSub, "syntheticMouseInput", isString) == "true";
            result.capabilities.absoluteMouseMovement = findRawValue(capsSub, "absoluteMouseMovement", isString) == "true";
            result.capabilities.getCursorPosition = findRawValue(capsSub, "getCursorPosition", isString) == "true";
        }

        result.recording = findRawValue(json, "recording", isString) == "true";
        result.recordingPaused = findRawValue(json, "recording_paused", isString) == "true";
        std::string_view eventCountStr = findRawValue(json, "recorded_event_count", isString);
        if (!eventCountStr.empty())
        {
            result.recordedEventCount = static_cast<uint32_t>(parseStringToInt(eventCountStr));
        }

        auto sequencePos = json.find("\"sequence\"");
        if (sequencePos != std::string_view::npos)
        {
            std::string_view seqSub = json.substr(sequencePos);
            RecordedSequence seq;
            
            bool isStr = false;
            std::string_view nameStr = findRawValue(seqSub, "name", isStr);
            seq.name = isStr ? jsonUnescape(nameStr) : std::string(nameStr);
            
            std::string_view startStr = findRawValue(seqSub, "start", isStr);
            seq.start = isStr ? jsonUnescape(startStr) : std::string(startStr);
            
            std::string_view repeatStr = findRawValue(seqSub, "repeat", isStr);
            seq.repeat = (repeatStr == "true");
            
            auto eventsPos = seqSub.find("\"events\"");
            if (eventsPos != std::string_view::npos)
            {
                auto arrayStart = seqSub.find('[', eventsPos);
                auto arrayEnd = seqSub.find(']', arrayStart);
                if (arrayStart != std::string_view::npos && arrayEnd != std::string_view::npos)
                {
                    std::string_view eventsArray = seqSub.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
                    size_t pos = 0;
                    while ((pos = eventsArray.find('{', pos)) != std::string_view::npos)
                    {
                        size_t endPos = eventsArray.find('}', pos);
                        if (endPos == std::string_view::npos) break;
                        
                        std::string_view eventJson = eventsArray.substr(pos, endPos - pos + 1);
                        RecordedEvent event;
                        
                        std::string_view typeStr = findRawValue(eventJson, "type", isStr);
                        event.type = recordedEventTypeFromString(isStr ? jsonUnescape(typeStr) : typeStr);
                        
                        std::string_view delayStr = findRawValue(eventJson, "delay", isStr);
                        event.delay = isStr ? jsonUnescape(delayStr) : std::string(delayStr);
                        
                        std::string_view keyStr = findRawValue(eventJson, "key", isStr);
                        if (!keyStr.empty()) event.key = isStr ? jsonUnescape(keyStr) : std::string(keyStr);
                        
                        std::string_view buttonStr = findRawValue(eventJson, "button", isStr);
                        if (!buttonStr.empty()) event.button = isStr ? jsonUnescape(buttonStr) : std::string(buttonStr);
                        
                        std::string_view xStr = findRawValue(eventJson, "x", isStr);
                        if (!xStr.empty()) event.x = parseStringToInt(xStr);
                        
                        std::string_view yStr = findRawValue(eventJson, "y", isStr);
                        if (!yStr.empty()) event.y = parseStringToInt(yStr);
                        
                        seq.events.push_back(std::move(event));
                        pos = endPos + 1;
                    }
                }
            }
            result.sequence = std::move(seq);
        }

        auto parseWindowInfo = [&](std::string_view json) {
            AppWindowInfo info;
            bool isStr = false;
            
            std::string_view procStr = findRawValue(json, "process_name", isStr);
            if (!procStr.empty()) info.processName = isStr ? jsonUnescape(procStr) : std::string(procStr);
            
            std::string_view titleStr = findRawValue(json, "window_title", isStr);
            if (!titleStr.empty()) info.windowTitle = isStr ? jsonUnescape(titleStr) : std::string(titleStr);
            
            std::string_view pidStr = findRawValue(json, "pid", isStr);
            if (!pidStr.empty()) {
                std::from_chars(pidStr.data(), pidStr.data() + pidStr.size(), info.pid);
            }
            
            std::string_view pathStr = findRawValue(json, "executable_path", isStr);
            if (!pathStr.empty()) info.executablePath = isStr ? jsonUnescape(pathStr) : std::string(pathStr);
            
            std::string_view backendStr = findRawValue(json, "backend_id", isStr);
            if (!backendStr.empty()) info.backendId = isStr ? jsonUnescape(backendStr) : std::string(backendStr);
            
            return info;
        };

        auto windowsPos = jsonLine.find("\"windows\"");
        if (windowsPos != std::string_view::npos)
        {
            auto arrayStart = jsonLine.find('[', windowsPos);
            auto arrayEnd = jsonLine.find(']', arrayStart);
            if (arrayStart != std::string_view::npos && arrayEnd != std::string_view::npos)
            {
                std::string_view windowsArray = jsonLine.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
                size_t pos = 0;
                while ((pos = windowsArray.find('{', pos)) != std::string_view::npos)
                {
                    size_t endPos = windowsArray.find('}', pos);
                    if (endPos == std::string_view::npos) break;
                    result.windows.push_back(parseWindowInfo(windowsArray.substr(pos, endPos - pos + 1)));
                    pos = endPos + 1;
                }
            }
        }

        auto foregroundPos = jsonLine.find("\"foreground_window\"");
        if (foregroundPos != std::string_view::npos)
        {
            auto objStart = jsonLine.find('{', foregroundPos);
            int braceCount = 0;
            size_t pos = objStart;
            while (pos != std::string_view::npos && pos < jsonLine.size())
            {
                if (jsonLine[pos] == '{') braceCount++;
                else if (jsonLine[pos] == '}') {
                    braceCount--;
                    if (braceCount == 0) break;
                }
                pos++;
            }
            if (objStart != std::string_view::npos && pos < jsonLine.size())
            {
                result.foregroundWindow = parseWindowInfo(jsonLine.substr(objStart, pos - objStart + 1));
            }
        }

        return result;
    }

} // namespace autoinput::services
