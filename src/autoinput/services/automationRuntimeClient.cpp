/**
* @file automationRuntimeClient.cpp
* @author djsquiddy
* @date August 2026
*/

#include "autoinput/services/automationRuntimeClient.h"
#include "autoinput/services/processTransport.h"
#include "autoinput/services/runtimeProtocol.h"
#include "autoinput/platform.h"
#include "autoinput/backendFactory.h"

#include "autoinput/arguments.h"
#include "autoinput/logger.h"
#include <exception>
#include <format>

namespace autoinput::services
{
    const char* statusToString(const RuntimeStatus status)
    {
        switch (status)
        {
        case RuntimeStatus::Stopped:  return "Stopped";
        case RuntimeStatus::Starting: return "Starting";
        case RuntimeStatus::Running:  return "Running";
        case RuntimeStatus::Paused:   return "Paused";
        case RuntimeStatus::Error:    return "Error";
        default:                      return "Unknown";
        }
    }

    ProcessAutomationRuntimeClient::ProcessAutomationRuntimeClient(std::unique_ptr<IProcessTransport> transport)
        : m_transport(std::move(transport))
    {
    }

    ProcessAutomationRuntimeClient::~ProcessAutomationRuntimeClient()
    {
        if (m_transport && m_transport->running())
        {
            // Try to shutdown gracefully
            AUTOINPUT_UNUSED(sendRequest(m_nextRequestId++, "shutdown"));
            m_transport->stop();
        }
    }

    RuntimeOperationResult ProcessAutomationRuntimeClient::start(std::string_view configName)
    {
        return sendRequest(m_nextRequestId++, "start", configName);
    }

    RuntimeOperationResult ProcessAutomationRuntimeClient::stop()
    {
        return sendRequest(m_nextRequestId++, "stop");
    }

    RuntimeOperationResult ProcessAutomationRuntimeClient::pause()
    {
        return sendRequest(m_nextRequestId++, "pause");
    }

    RuntimeOperationResult ProcessAutomationRuntimeClient::resume()
    {
        return sendRequest(m_nextRequestId++, "resume");
    }

    RuntimeOperationResult ProcessAutomationRuntimeClient::runCommand(std::string_view configName, std::string_view commandName)
    {
        if (!m_transport)
        {
            return { false, RuntimeStatus::Stopped, "Transport not initialized." };
        }

        const std::uint64_t id = m_nextRequestId++;
        const std::string request = buildRunCommandRequest(id, configName, commandName);

        if (!m_transport->writeLine(request))
        {
            m_status = RuntimeStatus::Error;
            const std::string error = m_transport->lastError();
            m_lastMessage = error.empty()
                ? "Failed to write run_command request to transport."
                : std::format("Failed to write run_command request to transport: {}", error);
            return { false, RuntimeStatus::Error, m_lastMessage };
        }

        auto responseJson = m_transport->readLine();
        if (!responseJson)
        {
            m_status = RuntimeStatus::Error;
            const std::string error = m_transport->lastError();
            m_lastMessage = error.empty()
                ? "Failed to read run_command response from transport."
                : std::format("Failed to read run_command response from transport: {}", error);
            return { false, RuntimeStatus::Error, m_lastMessage };
        }

        auto result = parseRuntimeResponse(*responseJson);
        m_status = result.status;
        m_lastMessage = result.message;
        this->m_recording = result.recording;
        this->m_recordingPaused = result.recordingPaused;
        this->m_recordedEventCount = result.recordedEventCount;
        return result;
    }

    RuntimeStatus ProcessAutomationRuntimeClient::getStatus() const
    {
        return m_status;
    }

    RuntimeOperationResult ProcessAutomationRuntimeClient::ping()
    {
        return sendRequest(m_nextRequestId++, "ping");
    }

    std::vector<AppWindowInfo> ProcessAutomationRuntimeClient::enumerateWindows()
    {
        auto res = sendRequest(m_nextRequestId++, "enumerate_windows");
        return res.windows;
    }

    std::optional<AppWindowInfo> ProcessAutomationRuntimeClient::getForegroundWindow()
    {
        auto res = sendRequest(m_nextRequestId++, "get_foreground_window");
        return res.foregroundWindow;
    }

    BackendCapabilities ProcessAutomationRuntimeClient::getBackendCapabilities() const
    {
        // Query diagnostics to get capabilities
        auto res = const_cast<ProcessAutomationRuntimeClient*>(this)->sendRequest(m_nextRequestId++, "get_diagnostics");
        return res.capabilities;
    }

    std::string ProcessAutomationRuntimeClient::getBackendName() const
    {
        auto res = const_cast<ProcessAutomationRuntimeClient*>(this)->sendRequest(m_nextRequestId++, "get_diagnostics");
        return res.backendName;
    }

    RuntimeOperationResult ProcessAutomationRuntimeClient::sendTestNotification(std::string_view title, std::string_view message, NotificationSeverity severity, std::optional<StatusNotificationMode> mode)
    {
        const std::string request = buildTestNotificationRequest(m_nextRequestId++, title, message, severity, mode);
        if (!m_transport->writeLine(request))
        {
            return { false, m_status, "Failed to write to transport." };
        }

        auto response = m_transport->readLine(std::chrono::milliseconds(5000));
        if (!response)
        {
            return { false, m_status, "Failed to read from transport (timeout)." };
        }

        auto res = parseRuntimeResponse(*response);
        m_status = res.status;
        m_lastMessage = res.message;
        this->m_recording = res.recording;
        this->m_recordingPaused = res.recordingPaused;
        this->m_recordedEventCount = res.recordedEventCount;
        return res;
    }

    RuntimeOperationResult ProcessAutomationRuntimeClient::startRecording(const SequenceConfig& config)
    {
        const std::uint64_t id = m_nextRequestId++;
        const std::string request = buildStartRecordingRequest(id, config);
        
        if (!m_transport->writeLine(request))
        {
            return { false, m_status, "Failed to write to transport." };
        }

        auto response = m_transport->readLine(std::chrono::milliseconds(5000));
        if (!response)
        {
            return { false, m_status, "Failed to read from transport (timeout)." };
        }

        auto res = parseRuntimeResponse(*response);
        m_status = res.status;
        m_lastMessage = res.message;
        this->m_recording = res.recording;
        this->m_recordingPaused = res.recordingPaused;
        this->m_recordedEventCount = res.recordedEventCount;
        return res;
    }

    RuntimeOperationResult ProcessAutomationRuntimeClient::stopRecording()
    {
        return sendRequest(m_nextRequestId++, "stop_recording");
    }

    RuntimeOperationResult ProcessAutomationRuntimeClient::pauseRecording()
    {
        return sendRequest(m_nextRequestId++, "pause_recording");
    }

    RuntimeOperationResult ProcessAutomationRuntimeClient::resumeRecording()
    {
        return sendRequest(m_nextRequestId++, "resume_recording");
    }

    RuntimeOperationResult ProcessAutomationRuntimeClient::discardRecording()
    {
        return sendRequest(m_nextRequestId++, "discard_recording");
    }

    std::optional<RecordedSequence> ProcessAutomationRuntimeClient::getRecordedSequence() const
    {
        const std::uint64_t id = m_nextRequestId++;
        const std::string request = buildGetRecordedSequenceRequest(id);
        
        if (!m_transport->writeLine(request))
        {
            return std::nullopt;
        }

        auto response = m_transport->readLine(std::chrono::milliseconds(10000)); // Longer timeout for large sequences
        if (!response)
        {
            return std::nullopt;
        }

        auto res = parseRuntimeResponse(*response);
        return res.sequence;
    }

    bool ProcessAutomationRuntimeClient::isRecording() const
    {
        return m_recording;
    }

    bool ProcessAutomationRuntimeClient::isRecordingPaused() const
    {
        return m_recordingPaused;
    }

    uint32_t ProcessAutomationRuntimeClient::getRecordedEventCount() const
    {
        return m_recordedEventCount;
    }

    std::string ProcessAutomationRuntimeClient::getActiveConfig() const
    {
        return ""; // Not implemented for process client yet
    }

    std::string ProcessAutomationRuntimeClient::getActiveCommand() const
    {
        return ""; // Not implemented for process client yet
    }

    std::string ProcessAutomationRuntimeClient::getLastMessage() const
    {
        return m_lastMessage;
    }

    bool ProcessAutomationRuntimeClient::isBackendAvailable() const
    {
        return m_transport && m_transport->running();
    }

    RuntimeOperationResult ProcessAutomationRuntimeClient::sendRequest(std::uint64_t id, std::string_view method, std::string_view config)
    {
        if (!m_transport)
        {
            return { false, RuntimeStatus::Error, "Transport is not available." };
        }

        if (!m_transport->running())
        {
            if (!m_transport->start())
            {
                std::string error = m_transport->lastError();
                return { false, RuntimeStatus::Error, error.empty() ? "Failed to start process transport." : std::format("Failed to start process transport: {}", error) };
            }
        }

        std::string requestJson;
        if (method == "start")
        {
            requestJson = buildStartRuntimeRequest(id, config);
        }
        else
        {
            requestJson = buildRuntimeRequest(id, method);
        }

        if (!m_transport->writeLine(requestJson))
        {
            m_status = RuntimeStatus::Error;
            const std::string error = m_transport->lastError();
            m_lastMessage = error.empty()
                ? "Failed to write request to transport."
                : std::format("Failed to write request to transport: {}", error);
            return {
                false,
                RuntimeStatus::Error,
                m_lastMessage
            };
        }

        auto responseJson = m_transport->readLine(std::chrono::milliseconds(5000));
        if (!responseJson)
        {
            m_status = RuntimeStatus::Error;
            const std::string error = m_transport->lastError();
            m_lastMessage = error.empty()
                ? "Failed to read response from transport."
                : std::format("Failed to read response from transport: {}", error);
            return {
                false,
                RuntimeStatus::Error,
                m_lastMessage
            };
        }

        auto result = parseRuntimeResponse(*responseJson);
        m_status = result.status;
        m_lastMessage = result.message;
        this->m_recording = result.recording;
        this->m_recordingPaused = result.recordingPaused;
        this->m_recordedEventCount = result.recordedEventCount;
        return result;
    }

    InProcessAutomationRuntimeClient::InProcessAutomationRuntimeClient(const IEnvironment& environment)
        : m_configService{ environment }
    {
        m_controller.setStatusCallback([this](const ProgramStatus& status)
        {
            if (status.active || status.recording)
            {
                m_status = RuntimeStatus::Running;
            }
            else
            {
                m_status = RuntimeStatus::Stopped;
            }
            
            if (status.active)
            {
                m_activeCommand = status.triggeredCommandName;
            }
            else
            {
                m_activeCommand.clear();
            }

            this->m_recording = status.recording;
            this->m_recordingPaused = status.recordingPaused;
            this->m_recordedEventCount = status.recordedEventCount;
        });
    }

    InProcessAutomationRuntimeClient::~InProcessAutomationRuntimeClient()
    {
        AUTOINPUT_UNUSED(stop());
    }

    RuntimeOperationResult InProcessAutomationRuntimeClient::start(const std::string_view configName)
    {
        if (m_controller.running())
        {
            return {
                .success = false,
                .status = RuntimeStatus::Running,
                .message = "Automation is already running."
            };
        }

        m_status = RuntimeStatus::Starting;

        try
        {
            std::unique_ptr<ProgramArguments> arguments = m_configService.loadConfigAsArguments(configName);
            if (!arguments)
            {
                m_status = RuntimeStatus::Error;
                return {
                    .success = false,
                    .status = RuntimeStatus::Error,
                    .message = std::format("Failed to load automation configuration. Config: {}", configName)
                };
            }

            if (!m_controller.start(std::move(*arguments)))
            {
                m_status = RuntimeStatus::Error;
                return {
                    .success = false,
                    .status = RuntimeStatus::Error,
                    .message = std::format("Failed to start automation runtime. Config: {}", configName)
                };
            }

            m_currentConfig = std::string(configName);
            m_status = RuntimeStatus::Running;
            m_lastMessage = std::format("Automation started with config: {}", configName);
            return {
                .success = true,
                .status = RuntimeStatus::Running,
                .message = m_lastMessage
            };
        }
        catch (const std::exception& e)
        {
            m_status = RuntimeStatus::Error;
            m_lastMessage = std::format("Unhandled exception while starting automation runtime: {}", e.what());
            return {
                .success = false,
                .status = RuntimeStatus::Error,
                .message = m_lastMessage
            };
        }
        catch (...)
        {
            m_status = RuntimeStatus::Error;
            m_lastMessage = "Unknown error while starting automation runtime.";
            return {
                .success = false,
                .status = RuntimeStatus::Error,
                .message = m_lastMessage
            };
        }
    }

    RuntimeOperationResult InProcessAutomationRuntimeClient::stop()
    {
        try
        {
            if (m_controller.running())
            {
                m_controller.stop();
            }

            m_status = RuntimeStatus::Stopped;
            m_currentConfig.clear();
            m_activeCommand.clear();
            m_lastMessage = "Automation stopped.";
            return {
                .success = true,
                .status = RuntimeStatus::Stopped,
                .message = m_lastMessage
            };
        }
        catch (const std::exception& e)
        {
            m_status = RuntimeStatus::Error;
            m_lastMessage = e.what();
            Logger::error("Unhandled exception while stopping automation runtime: {}", m_lastMessage);
            return {
                .success = false,
                .status = RuntimeStatus::Error,
                .message = m_lastMessage
            };
        }
        catch (...)
        {
            m_status = RuntimeStatus::Error;
            m_lastMessage = "Unknown error while stopping automation runtime.";
            Logger::error(m_lastMessage);
            return {
                .success = false,
                .status = RuntimeStatus::Error,
                .message = m_lastMessage
            };
        }
    }

    RuntimeOperationResult InProcessAutomationRuntimeClient::pause()
    {
        return {
            .success = false,
            .status = getStatus(),
            .message = "Pause is not supported by the in-process automation runtime yet."
        };
    }

    RuntimeOperationResult InProcessAutomationRuntimeClient::resume()
    {
        m_controller.resume();
        m_status = RuntimeStatus::Running;
        m_lastMessage = "Automation resumed.";
        return { true, RuntimeStatus::Running, m_lastMessage };
    }

    RuntimeOperationResult InProcessAutomationRuntimeClient::runCommand(std::string_view configName, std::string_view commandName)
    {
        try
        {
            if (m_status != RuntimeStatus::Running || m_currentConfig != configName)
            {
                auto startResult = start(configName);
                if (!startResult.success)
                {
                    return startResult;
                }
            }

            m_controller.runCommand(commandName);
            m_lastMessage = std::format("Command '{}' triggered.", commandName);
            return { true, m_status, m_lastMessage };
        }
        catch (const std::exception& e)
        {
            m_lastMessage = std::format("Error triggering command: {}", e.what());
            return { false, m_status, m_lastMessage };
        }
    }

    RuntimeStatus InProcessAutomationRuntimeClient::getStatus() const
    {
        if (m_controller.running())
        {
            return RuntimeStatus::Running;
        }

        if (m_status == RuntimeStatus::Error)
        {
            return RuntimeStatus::Error;
        }

        return RuntimeStatus::Stopped;
    }

    std::string InProcessAutomationRuntimeClient::getActiveConfig() const
    {
        return m_currentConfig;
    }

    std::string InProcessAutomationRuntimeClient::getActiveCommand() const
    {
        return m_activeCommand;
    }

    std::string InProcessAutomationRuntimeClient::getLastMessage() const
    {
        return m_lastMessage;
    }

    bool InProcessAutomationRuntimeClient::isBackendAvailable() const
    {
        return true; // In-process is always available if we got here
    }

    RuntimeOperationResult InProcessAutomationRuntimeClient::ping()
    {
        return { true, m_status, "Pong (In-Process)" };
    }

    BackendCapabilities InProcessAutomationRuntimeClient::getBackendCapabilities() const
    {
        if (auto* backend = m_controller.getBackend())
        {
            return backend->capabilities();
        }
        
        // If not running, try to get a temporary backend for detection
        auto tempBackend = BackendFactory::createPlatformBackend();
        if (tempBackend)
        {
            return tempBackend->capabilities();
        }

        return {};
    }

    std::string InProcessAutomationRuntimeClient::getBackendName() const
    {
        if (auto* backend = m_controller.getBackend())
        {
            return backend->getName();
        }

        auto tempBackend = BackendFactory::createPlatformBackend();
        if (tempBackend)
        {
            return tempBackend->getName();
        }

        return "Unknown";
    }

    RuntimeOperationResult InProcessAutomationRuntimeClient::sendTestNotification(std::string_view title, std::string_view message, NotificationSeverity severity, std::optional<StatusNotificationMode> mode)
    {
        StatusNotificationMode finalMode = mode.value_or(StatusNotificationMode::Desktop);
        NotificationService svc(finalMode, false);
        // Add default sinks
        if ((finalMode & StatusNotificationMode::Desktop) != StatusNotificationMode::Off)
        {
#ifdef _WIN32
            svc.addSink(platform::createDesktopNotificationSink());
#endif
        }
        svc.notify(std::string(title), std::string(message), severity);
        return { true, m_status, "Notification sent" };
    }

    RuntimeOperationResult InProcessAutomationRuntimeClient::startRecording(const SequenceConfig& config)
    {
        m_controller.startRecording(config);
        return { true, m_status, "Recording started." };
    }

    RuntimeOperationResult InProcessAutomationRuntimeClient::stopRecording()
    {
        m_controller.stopRecording();
        return { true, m_status, "Recording stopped." };
    }

    RuntimeOperationResult InProcessAutomationRuntimeClient::pauseRecording()
    {
        m_controller.pauseRecording();
        return { true, m_status, "Recording paused." };
    }

    RuntimeOperationResult InProcessAutomationRuntimeClient::resumeRecording()
    {
        m_controller.resumeRecording();
        return { true, m_status, "Recording resumed." };
    }

    RuntimeOperationResult InProcessAutomationRuntimeClient::discardRecording()
    {
        m_controller.discardRecording();
        return { true, m_status, "Recording discarded." };
    }

    std::optional<RecordedSequence> InProcessAutomationRuntimeClient::getRecordedSequence() const
    {
        auto seq = m_controller.getRecordedSequence();
        if (seq) return *seq;
        return std::nullopt;
    }

    bool InProcessAutomationRuntimeClient::isRecording() const
    {
        return m_recording;
    }

    bool InProcessAutomationRuntimeClient::isRecordingPaused() const
    {
        return m_recordingPaused;
    }

    uint32_t InProcessAutomationRuntimeClient::getRecordedEventCount() const
    {
        return m_recordedEventCount;
    }

    std::vector<AppWindowInfo> InProcessAutomationRuntimeClient::enumerateWindows()
    {
        auto* backend = m_controller.getBackend();
        if (backend)
        {
            return backend->enumerateWindows();
        }
        return {};
    }

    std::optional<AppWindowInfo> InProcessAutomationRuntimeClient::getForegroundWindow()
    {
        auto* backend = m_controller.getBackend();
        if (backend)
        {
            return backend->getForegroundWindow();
        }
        return std::nullopt;
    }

    std::unique_ptr<IAutomationRuntimeClient> createAutomationRuntimeClient(AutomationRuntimeClientMode mode)
    {
        switch (mode)
        {
            case AutomationRuntimeClientMode::InProcess:
                return std::make_unique<InProcessAutomationRuntimeClient>(SystemEnvironment::instance());
        case AutomationRuntimeClientMode::Process:
                return createProcessAutomationRuntimeClient(SystemEnvironment::instance());
            default:
                return nullptr;
        }
    }

    std::unique_ptr<IAutomationRuntimeClient> createProcessAutomationRuntimeClient(const IEnvironment& environment)
    {
        auto transport = std::make_unique<StdioProcessTransport>(
            environment.executablePath(),
            std::vector<std::string>{ "serve", "--stdio" }
        );
        return std::make_unique<ProcessAutomationRuntimeClient>(std::move(transport));
    }
}