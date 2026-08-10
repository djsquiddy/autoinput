/**
 * @file automationRuntimeClient.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_SERVICES_AUTOMATIONRUNTIMECLIENT_H
#define INCLUDE_AUTOINPUT_SERVICES_AUTOMATIONRUNTIMECLIENT_H
#pragma once

#include "autoinput/app/automationController.h"
#include "autoinput/platform/environment.h"
#include "autoinput/services/configService.h"
#include "autoinput/support/types.h"
#include "autoinput/platform/backend.h"
#include <memory>
#include <string>
#include <string_view>
#include <filesystem>

namespace autoinput::services
{
    class IProcessTransport;

    /**
     * @brief The current status of the automation runtime.
     */
    enum class RuntimeStatus : u8
    {
        Stopped,    /**< Runtime is not running. */
        Starting,   /**< Runtime is in the process of starting. */
        Running,    /**< Runtime is actively running. */
        Paused,     /**< Runtime is running but automation is paused. */
        Error       /**< Runtime is in an error state. */
    };

    /**
     * @brief Converts a RuntimeStatus to a string.
     * @param status The status.
     * @return The string representation.
     */
    const char* statusToString(RuntimeStatus status);

    /**
     * @brief The result of a runtime operation.
     */
    struct RuntimeOperationResult
    {
        bool success{ false };                          /**< Whether the operation succeeded. */
        RuntimeStatus status{ RuntimeStatus::Stopped }; /**< The status of the runtime after the operation. */
        std::string message{};                          /**< An optional message describing the result or error. */
        std::string backendName{};                      /**< The name of the platform backend in use. */
        BackendCapabilities capabilities{};             /**< The capabilities of the backend. */
        bool recording{ false };                        /**< Whether a recording is currently in progress. */
        bool recordingPaused{ false };                  /**< Whether the current recording is paused. */
        uint32_t recordedEventCount{ 0 };               /**< The number of events recorded so far. */
        std::optional<RecordedSequence> sequence;       /**< The recorded sequence data, if applicable. */
        std::vector<AppWindowInfo> windows;             /**< List of open windows, if requested. */
        std::optional<AppWindowInfo> foregroundWindow;  /**< The current foreground window, if requested. */
    };

    /**
     * @brief Interface for clients interacting with the automation runtime.
     */
    class IAutomationRuntimeClient
    {
    public:
        virtual ~IAutomationRuntimeClient() = default;

        /**
         * @brief Starts the automation runtime with a specific configuration.
         * @param configName The name or path of the configuration to load.
         * @return The result of the operation.
         */
        [[nodiscard]] virtual RuntimeOperationResult start(std::string_view configName) = 0;
        
        /**
         * @brief Stops the automation runtime.
         * @return The result of the operation.
         */
        [[nodiscard]] virtual RuntimeOperationResult stop() = 0;
        
        /**
         * @brief Pauses automation actions.
         * @return The result of the operation.
         */
        [[nodiscard]] virtual RuntimeOperationResult pause() = 0;
        
        /**
         * @brief Resumes automation actions.
         * @return The result of the operation.
         */
        [[nodiscard]] virtual RuntimeOperationResult resume() = 0;
        
        /**
         * @brief Executes a specific command.
         * @param configName The configuration containing the command.
         * @param commandName The name of the command to execute.
         * @return The result of the operation.
         */
        [[nodiscard]] virtual RuntimeOperationResult runCommand(std::string_view configName, std::string_view commandName) = 0;
        
        /**
         * @brief Gets the current status of the runtime.
         * @return The runtime status.
         */
        [[nodiscard]] virtual RuntimeStatus getStatus() const = 0;
        
        /**
         * @brief Gets the name of the currently active configuration.
         * @return The configuration name.
         */
        [[nodiscard]] virtual std::string getActiveConfig() const = 0;
        
        /**
         * @brief Gets the name of the currently active command.
         * @return The command name.
         */
        [[nodiscard]] virtual std::string getActiveCommand() const = 0;
        
        /**
         * @brief Gets the last message from the runtime.
         * @return The message string.
         */
        [[nodiscard]] virtual std::string getLastMessage() const = 0;
        
        /**
         * @brief Checks if a platform backend is available.
         * @return True if available.
         */
        [[nodiscard]] virtual bool isBackendAvailable() const = 0;

        /**
         * @brief Pings the runtime to check connectivity.
         * @return The result of the operation.
         */
        [[nodiscard]] virtual RuntimeOperationResult ping() = 0;
        /**
         * @brief Enumerates all visible windows.
         * @return A vector of AppWindowInfo.
         */
        [[nodiscard]] virtual std::vector<AppWindowInfo> enumerateWindows() = 0;

        /**
         * @brief Gets the information of the current foreground window.
         * @return An AppWindowInfo struct, or std::nullopt if not available.
         */
        [[nodiscard]] virtual std::optional<AppWindowInfo> getForegroundWindow() = 0;

        /**
         * @brief Gets the capabilities of the backend.
         * @return The backend capabilities.
         */
        [[nodiscard]] virtual BackendCapabilities getBackendCapabilities() const = 0;
        
        /**
         * @brief Gets the name of the backend.
         * @return The backend name.
         */
        [[nodiscard]] virtual std::string getBackendName() const = 0;
        
        /**
         * @brief Sends a test notification through the runtime.
         * @param title The title of the notification.
         * @param message The message body.
         * @param severity The severity level.
         * @param mode Optional override for the notification mode.
         * @return The result of the operation.
         */
        [[nodiscard]] virtual RuntimeOperationResult sendTestNotification(std::string_view title, std::string_view message, NotificationSeverity severity = NotificationSeverity::Info, std::optional<StatusNotificationMode> mode = std::nullopt) = 0;

        /**
         * @brief Starts recording a sequence.
         * @param config The recording configuration.
         * @return The result of the operation.
         */
        virtual RuntimeOperationResult startRecording(const SequenceConfig& config) = 0;
        
        /**
         * @brief Stops and saves the current recording.
         * @return The result of the operation.
         */
        virtual RuntimeOperationResult stopRecording() = 0;
        
        /**
         * @brief Pauses the current recording.
         * @return The result of the operation.
         */
        virtual RuntimeOperationResult pauseRecording() = 0;
        
        /**
         * @brief Resumes the current recording.
         * @return The result of the operation.
         */
        virtual RuntimeOperationResult resumeRecording() = 0;
        
        /**
         * @brief Discards the current recording.
         * @return The result of the operation.
         */
        virtual RuntimeOperationResult discardRecording() = 0;
        
        /**
         * @brief Gets the recorded sequence.
         * @return An optional RecordedSequence.
         */
        virtual std::optional<RecordedSequence> getRecordedSequence() const = 0;

        /**
         * @brief Checks if a recording is in progress.
         * @return True if recording.
         */
        [[nodiscard]] virtual bool isRecording() const = 0;
        
        /**
         * @brief Checks if the current recording is paused.
         * @return True if paused.
         */
        [[nodiscard]] virtual bool isRecordingPaused() const = 0;
        
        /**
         * @brief Gets the number of events in the current recording.
         * @return The event count.
         */
        [[nodiscard]] virtual uint32_t getRecordedEventCount() const = 0;
    };

    /**
     * @brief Implementation of IAutomationRuntimeClient that communicates with a separate runtime process.
     */
    class ProcessAutomationRuntimeClient final : public IAutomationRuntimeClient
    {
    public:
        /**
         * @brief Constructs a ProcessAutomationRuntimeClient.
         * @param transport The transport mechanism for communication.
         */
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
        [[nodiscard]] std::vector<AppWindowInfo> enumerateWindows() override;
        [[nodiscard]] std::optional<AppWindowInfo> getForegroundWindow() override;
        [[nodiscard]] BackendCapabilities getBackendCapabilities() const override;
        [[nodiscard]] std::string getBackendName() const override;
        [[nodiscard]] RuntimeOperationResult sendTestNotification(std::string_view title, std::string_view message, NotificationSeverity severity = NotificationSeverity::Info, std::optional<StatusNotificationMode> mode = std::nullopt) override;

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

    /**
     * @brief Implementation of IAutomationRuntimeClient that runs the automation controller in the same process.
     */
    class InProcessAutomationRuntimeClient final : public IAutomationRuntimeClient
    {
    public:
        /**
         * @brief Constructs an InProcessAutomationRuntimeClient.
         * @param environment The environment to use.
         */
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
        [[nodiscard]] std::vector<AppWindowInfo> enumerateWindows() override;
        [[nodiscard]] std::optional<AppWindowInfo> getForegroundWindow() override;
        [[nodiscard]] BackendCapabilities getBackendCapabilities() const override;
        [[nodiscard]] std::string getBackendName() const override;
        [[nodiscard]] RuntimeOperationResult sendTestNotification(std::string_view title, std::string_view message, NotificationSeverity severity = NotificationSeverity::Info, std::optional<StatusNotificationMode> mode = std::nullopt) override;

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
#endif // INCLUDE_AUTOINPUT_SERVICES_AUTOMATIONRUNTIMECLIENT_H
