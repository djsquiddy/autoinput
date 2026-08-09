/**
* @file automationRuntimeClient.cpp
* @author djsquiddy
* @date August 2026
*/

#include "autoinput/services/automationRuntimeClient.h"


#include "autoinput/arguments.h"
#include "autoinput/logger.h"
#include <exception>

namespace autoinput::services
{
    namespace
    {
        constexpr auto ProcessClientNotImplementedMessage =
            "Process automation runtime client is not implemented yet.";
    }

    RuntimeOperationResult ProcessAutomationRuntimeClient::start(std::string_view configName)
    {
        return {
            .success = false,
            .status = RuntimeStatus::Error,
            .message = std::format(
                "{} Cannot start config: {}",
                ProcessClientNotImplementedMessage,
                configName
            )
        };
    }

    RuntimeOperationResult ProcessAutomationRuntimeClient::stop()
    {
        return {
            .success = false,
            .status = RuntimeStatus::Error,
            .message = ProcessClientNotImplementedMessage
        };
    }

    RuntimeOperationResult ProcessAutomationRuntimeClient::pause()
    {
        return {
            .success = false,
            .status = RuntimeStatus::Error,
            .message = ProcessClientNotImplementedMessage
        };
    }

    RuntimeOperationResult ProcessAutomationRuntimeClient::resume()
    {
        return {
            .success = false,
            .status = RuntimeStatus::Error,
            .message = ProcessClientNotImplementedMessage
        };
    }

    RuntimeStatus ProcessAutomationRuntimeClient::getStatus() const
    {
        return RuntimeStatus::Stopped;
    }

    InProcessAutomationRuntimeClient::InProcessAutomationRuntimeClient(const IEnvironment& environment)
        : m_configService{ environment }
    {
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
            return {
                .success = true,
                .status = RuntimeStatus::Running,
                .message = std::format("Automation started with config: {}", configName)
            };
        }
        catch (const std::exception& e)
        {
            m_status = RuntimeStatus::Error;
            return {
                .success = false,
                .status = RuntimeStatus::Error,
                .message = std::format("Unhandled exception while starting automation runtime: {}", e.what())
            };
        }
        catch (...)
        {
            m_status = RuntimeStatus::Error;
            return {
                .success = false,
                .status = RuntimeStatus::Error,
                .message = "Unknown error while starting automation runtime."
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
            return {
                .success = true,
                .status = RuntimeStatus::Stopped,
                .message = "Automation stopped."
            };
        }
        catch (const std::exception& e)
        {
            m_status = RuntimeStatus::Error;
            Logger::error("Unhandled exception while stopping automation runtime: {}", e.what());
            return {
                .success = false,
                .status = RuntimeStatus::Error,
                .message = e.what()
            };
        }
        catch (...)
        {
            m_status = RuntimeStatus::Error;
            Logger::error("Unknown error while stopping automation runtime.");
            return {
                .success = false,
                .status = RuntimeStatus::Error,
                .message = "Unknown error while stopping automation runtime."
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

    std::string_view InProcessAutomationRuntimeClient::getCurrentConfig() const
    {
        return m_currentConfig;
    }

}