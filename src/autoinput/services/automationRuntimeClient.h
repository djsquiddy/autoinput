/**
* @file automationRuntimeClient.h
* @author djsquiddy
* @date August 2026
*/

#ifndef INCLUDE_AUTOINPUT_SERVICE_AUTOMATION_RUNTIME_CLIENT_H
#define INCLUDE_AUTOINPUT_SERVICE_AUTOMATION_RUNTIME_CLIENT_H
#pragma once

#include "autoinput/automationController.h"
#include "autoinput/environment.h"
#include "autoinput/services/configService.h"
#include "autoinput/types.h"
#include <memory>
#include <string>
#include <string_view>
#include <filesystem>

namespace autoinput::services
{
    class IProcessTransport;

    enum class RuntimeStatus : u8
    {
        Stopped,
        Starting,
        Running,
        Paused,
        Error
    };

    struct RuntimeOperationResult
    {
        bool success{ false };
        RuntimeStatus status{ RuntimeStatus::Stopped };
        std::string message{};
    };

    class IAutomationRuntimeClient
    {
    public:
        virtual ~IAutomationRuntimeClient() = default;

        [[nodiscard]] virtual RuntimeOperationResult start(std::string_view configName) = 0;
        [[nodiscard]] virtual RuntimeOperationResult stop() = 0;
        [[nodiscard]] virtual RuntimeOperationResult pause() = 0;
        [[nodiscard]] virtual RuntimeOperationResult resume() = 0;
        [[nodiscard]] virtual RuntimeStatus getStatus() const = 0;
    };

    class ProcessAutomationRuntimeClient final : public IAutomationRuntimeClient
    {
    public:
        explicit ProcessAutomationRuntimeClient(std::unique_ptr<IProcessTransport> transport);
        ~ProcessAutomationRuntimeClient() override;
        
        [[nodiscard]] RuntimeOperationResult start(std::string_view configName) override;
        [[nodiscard]] RuntimeOperationResult stop() override;
        [[nodiscard]] RuntimeOperationResult pause() override;
        [[nodiscard]] RuntimeOperationResult resume() override;
        [[nodiscard]] RuntimeStatus getStatus() const override;

    private:
        RuntimeOperationResult sendRequest(std::uint64_t id, std::string_view method, std::string_view config = "");
        
        std::unique_ptr<IProcessTransport> m_transport;
        mutable RuntimeStatus m_status{ RuntimeStatus::Stopped };
        mutable std::uint64_t m_nextRequestId{ 1 };
    };

    class InProcessAutomationRuntimeClient final : public IAutomationRuntimeClient
    {
    public:
        explicit InProcessAutomationRuntimeClient(const IEnvironment& environment = SystemEnvironment::instance());
        ~InProcessAutomationRuntimeClient() override;

        [[nodiscard]] RuntimeOperationResult start(std::string_view configName) override;
        [[nodiscard]] RuntimeOperationResult stop() override;
        [[nodiscard]] RuntimeOperationResult pause() override;
        [[nodiscard]] RuntimeOperationResult resume() override;
        [[nodiscard]] RuntimeStatus getStatus() const override;

        [[nodiscard]] std::string_view getCurrentConfig() const;

    private:
        ConfigService m_configService;
        AutomationController m_controller;
        RuntimeStatus m_status{ RuntimeStatus::Stopped };
        std::string m_currentConfig;
    };

    enum class AutomationRuntimeClientMode
    {
        InProcess,
        Process
    };

    std::unique_ptr<IAutomationRuntimeClient> createAutomationRuntimeClient(AutomationRuntimeClientMode mode);
    std::unique_ptr<IAutomationRuntimeClient> createProcessAutomationRuntimeClient(const IEnvironment& environment);
}
#endif // INCLUDE_AUTOINPUT_SERVICE_AUTOMATION_RUNTIME_CLIENT_H
