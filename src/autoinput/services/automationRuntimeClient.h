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
#include "autoinput/backend.h"
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
        std::string backendName{};
        BackendCapabilities capabilities{};
        bool recording{ false };
        bool recordingPaused{ false };
        uint32_t recordedEventCount{ 0 };
        std::optional<RecordedSequence> sequence;
    };

    class IAutomationRuntimeClient
    {
    public:
        virtual ~IAutomationRuntimeClient() = default;

        [[nodiscard]] virtual RuntimeOperationResult start(std::string_view configName) = 0;
        [[nodiscard]] virtual RuntimeOperationResult stop() = 0;
        [[nodiscard]] virtual RuntimeOperationResult pause() = 0;
        [[nodiscard]] virtual RuntimeOperationResult resume() = 0;
        [[nodiscard]] virtual RuntimeOperationResult runCommand(std::string_view configName, std::string_view commandName) = 0;
        [[nodiscard]] virtual RuntimeStatus getStatus() const = 0;
        
        [[nodiscard]] virtual std::string getActiveConfig() const = 0;
        [[nodiscard]] virtual std::string getActiveCommand() const = 0;
        [[nodiscard]] virtual std::string getLastMessage() const = 0;
        [[nodiscard]] virtual bool isBackendAvailable() const = 0;

        [[nodiscard]] virtual RuntimeOperationResult ping() = 0;
        [[nodiscard]] virtual BackendCapabilities getBackendCapabilities() const = 0;
        [[nodiscard]] virtual std::string getBackendName() const = 0;
        [[nodiscard]] virtual RuntimeOperationResult sendTestNotification(std::string_view title, std::string_view message) = 0;

        virtual RuntimeOperationResult startRecording(const SequenceConfig& config) = 0;
        virtual RuntimeOperationResult stopRecording() = 0;
        virtual RuntimeOperationResult pauseRecording() = 0;
        virtual RuntimeOperationResult resumeRecording() = 0;
        virtual RuntimeOperationResult discardRecording() = 0;
        virtual std::optional<RecordedSequence> getRecordedSequence() const = 0;

        [[nodiscard]] virtual bool isRecording() const = 0;
        [[nodiscard]] virtual bool isRecordingPaused() const = 0;
        [[nodiscard]] virtual uint32_t getRecordedEventCount() const = 0;
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
        [[nodiscard]] RuntimeOperationResult runCommand(std::string_view configName, std::string_view commandName) override;
        [[nodiscard]] RuntimeStatus getStatus() const override;

        [[nodiscard]] std::string getActiveConfig() const override;
        [[nodiscard]] std::string getActiveCommand() const override;
        [[nodiscard]] std::string getLastMessage() const override;
        [[nodiscard]] bool isBackendAvailable() const override;

        [[nodiscard]] RuntimeOperationResult ping() override;
        [[nodiscard]] BackendCapabilities getBackendCapabilities() const override;
        [[nodiscard]] std::string getBackendName() const override;
        [[nodiscard]] RuntimeOperationResult sendTestNotification(std::string_view title, std::string_view message) override;

        RuntimeOperationResult startRecording(const SequenceConfig& config) override;
        RuntimeOperationResult stopRecording() override;
        RuntimeOperationResult pauseRecording() override;
        RuntimeOperationResult resumeRecording() override;
        RuntimeOperationResult discardRecording() override;
        std::optional<RecordedSequence> getRecordedSequence() const override;

        [[nodiscard]] bool isRecording() const override;
        [[nodiscard]] bool isRecordingPaused() const override;
        [[nodiscard]] uint32_t getRecordedEventCount() const override;

    private:
        RuntimeOperationResult sendRequest(std::uint64_t id, std::string_view method, std::string_view config = "");
        
        std::unique_ptr<IProcessTransport> m_transport;
        mutable RuntimeStatus m_status{ RuntimeStatus::Stopped };
        mutable std::uint64_t m_nextRequestId{ 1 };
        mutable std::string m_lastMessage;
        bool m_recording{ false };
        bool m_recordingPaused{ false };
        uint32_t m_recordedEventCount{ 0 };
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
        [[nodiscard]] RuntimeOperationResult runCommand(std::string_view configName, std::string_view commandName) override;
        [[nodiscard]] RuntimeStatus getStatus() const override;

        [[nodiscard]] std::string getActiveConfig() const override;
        [[nodiscard]] std::string getActiveCommand() const override;
        [[nodiscard]] std::string getLastMessage() const override;
        [[nodiscard]] bool isBackendAvailable() const override;

        [[nodiscard]] RuntimeOperationResult ping() override;
        [[nodiscard]] BackendCapabilities getBackendCapabilities() const override;
        [[nodiscard]] std::string getBackendName() const override;
        [[nodiscard]] RuntimeOperationResult sendTestNotification(std::string_view title, std::string_view message) override;

        RuntimeOperationResult startRecording(const SequenceConfig& config) override;
        RuntimeOperationResult stopRecording() override;
        RuntimeOperationResult pauseRecording() override;
        RuntimeOperationResult resumeRecording() override;
        RuntimeOperationResult discardRecording() override;
        std::optional<RecordedSequence> getRecordedSequence() const override;

        [[nodiscard]] bool isRecording() const override;
        [[nodiscard]] bool isRecordingPaused() const override;
        [[nodiscard]] uint32_t getRecordedEventCount() const override;

    private:
        ConfigService m_configService;
        AutomationController m_controller;
        RuntimeStatus m_status{ RuntimeStatus::Stopped };
        std::string m_currentConfig;
        std::string m_activeCommand;
        std::string m_lastMessage;
        bool m_recording{ false };
        bool m_recordingPaused{ false };
        uint32_t m_recordedEventCount{ 0 };
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
