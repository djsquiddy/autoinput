/**
* @file automationRuntimeClient.cpp
* @author djsquiddy
* @date August 2026
*/

#include "autoinput/services/automationRuntimeClient.h"
#include "autoinput/services/processTransport.h"
#include "autoinput/services/runtimeProtocol.h"
#include "autoinput/platform.h"

#include "autoinput/arguments.h"
#include "autoinput/logger.h"
#include <exception>
#include <format>

namespace autoinput::services
{
    namespace
    {
        constexpr auto ProcessClientNotImplementedMessage =
            "Process automation runtime client is not implemented yet.";
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

    RuntimeStatus ProcessAutomationRuntimeClient::getStatus() const
    {
        return m_status;
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
        return result;
    }

    InProcessAutomationRuntimeClient::InProcessAutomationRuntimeClient(const IEnvironment& environment)
        : m_configService{ environment }
    {
        m_controller.setStatusCallback([this](const ProgramStatus& status)
        {
            if (status.active)
            {
                m_activeCommand = status.triggeredCommandName;
            }
            else
            {
                m_activeCommand.clear();
            }
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
        return {
            .success = false,
            .status = getStatus(),
            .message = "Resume is not supported by the in-process automation runtime yet."
        };
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

    std::unique_ptr<IAutomationRuntimeClient> createAutomationRuntimeClient(AutomationRuntimeClientMode mode)
    {
        switch (mode)
        {
            case AutomationRuntimeClientMode::InProcess:
                return std::make_unique<InProcessAutomationRuntimeClient>();
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