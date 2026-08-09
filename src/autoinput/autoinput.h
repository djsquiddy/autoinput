/**
 * @file autoInput.h
 * @author djsquiddy
 * @date March 2026
 */
#ifndef INCLUDE_AUTOINPUT_AUTOINPUT_H
#define INCLUDE_AUTOINPUT_AUTOINPUT_H
#pragma once

#include <utility>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <thread>
#include <optional>

#include "autoinput/arguments.h"
#include "autoinput/backend.h"
#include "autoinput/mouse.h"
#include "autoinput/keyboard.h"
#include "autoinput/keyInfo.h"
#include "autoinput/handlerState.h"
#include "autoinput/types.h"
#include "autoinput/notifications.h"
#include "autoinput/sequence.h"
#include "autoinput/sequenceRecorder.h"
#include "autoinput/waitDelay.h"

namespace autoinput
{
    struct ProgramStatus
    {
        bool active = false;
        std::string triggeredCommandName;
        std::optional<bool> triggeredCommandActive;

        bool recording = false;
        bool recordingPaused = false;
        uint32_t recordedEventCount = 0;
    };

    using StatusCallback = std::function<void(const ProgramStatus&)>;

    class Program
    {
    public:
        Program() = default;
        /**
         * @brief Constructs a Program with a specified platform backend.
         * @param backend A unique pointer to the platform backend to use.
         */
        explicit Program(std::unique_ptr<IPlatformBackend> backend);

        /**
         * @brief Gets the program arguments.
         * @return Reference to the ProgramArguments.
         */
        ProgramArguments& arguments() { return m_arguments; }

        /**
         * @brief Sets the platform backend for the program.
         * @param backend A unique pointer to the platform backend to set.
         */
        void setBackend(std::unique_ptr<IPlatformBackend> backend);

        /**
         * @brief Gets the current platform backend.
         * @return Pointer to the IPlatformBackend, or nullptr if none set.
         */
        [[nodiscard]] IPlatformBackend* getBackend() const { return m_backend.get(); }

        /**
         * @brief Releases ownership of the platform backend.
         * @return A unique pointer to the released IPlatformBackend.
         */
        std::unique_ptr<IPlatformBackend> releaseBackend() { return std::move(m_backend); }

        /**
         * @brief Initializes the program, setting up handlers from arguments.
         * @return True if initialization was successful, false otherwise.
         */
        bool init();
        
        /**
         * @brief Installs system-level input hooks.
         * @return True if hooks were successfully installed, false otherwise.
         */
        bool installHooks();

        /**
         * @brief Runs the main event listener loop.
         */
        void runListener();

        /**
         * @brief Requests the program to stop its listener loop.
         */
        void requestStop();

        /**
         * @brief Cleans up resources, including uninstalling hooks.
         */
        void cleanup();

        /**
         * @brief Processes a keyboard input event.
         * @param input The keyboard input data.
         * @return True if the event was handled, false otherwise.
         */
        bool processKeyEvent(KeyboardInput&& input);

        /**
         * @brief Processes a mouse input event.
         * @param input The mouse input data.
         * @return True if the event was handled, false otherwise.
         */
        bool processMouseEvent(const MouseInput& input);

        /**
         * @brief Starts the program's main execution based on the provided key information.
         * @param keyInfo The key information to start with.
         */
        void start(const KeyInfo& keyInfo);

        /**
         * @brief Ends the program's main execution.
         */
        void end();

        /**
         * @brief Programmatically runs a command by name.
         * @param name The name of the command to run.
         */
        void runCommand(std::string_view name);

        /**
         * @brief Gets the list of key information configured.
         * @return A const reference to the vector of KeyInfo.
         */
        [[nodiscard]] const std::vector<KeyInfo>& getKeyInfo() const { return m_keyInfo; }

#ifdef AUTOINPUT_TESTING
        /**
         * @brief Gets the mouse handlers (for testing).
         * @return Reference to the mouse handlers map.
         */
        auto& getMouseHandlers() { return m_mouseHandlers; }

        /**
         * @brief Gets the key handlers (for testing).
         * @return Reference to the key handlers map.
         */
        auto& getKeyHandlers() { return m_keyHandlers; }

        /**
         * @brief Gets the sequence handlers (for testing).
         * @return Reference to the sequence handlers map.
         */
        auto& getSequenceHandlers() { return m_sequenceHandlers; }

        /**
         * @brief Gets the last recorded active indicator state (for testing).
         * @return The last indicator state.
         */
        [[nodiscard]] bool getLastIsActiveIndicator() const { return m_lastIsActiveIndicator; }
        [[nodiscard]] std::string getLastTriggeredCommandName() const { return m_lastTriggeredCommandName; }
        [[nodiscard]] std::optional<bool> getLastTriggeredCommandActive() const { return m_lastTriggeredCommandActive; }

        /**
         * @brief Sets the active application name for testing purposes.
         * @param app The application name.
         */
        void setTestActiveApp(std::string app) { m_testActiveApp = std::move(app); }

        /**
         * @brief Gets the notification service (for testing).
         * @return Pointer to the NotificationService.
         */
        NotificationService* getNotificationService() const { return m_notificationService.get(); }
#endif

        /**
         * @brief Prints information about the program to the console.
         */
        void printProgramInfo() const;

        /**
         * @brief Checks if the currently active application is blacklisted.
         * @return True if blacklisted, false otherwise.
         */
        [[nodiscard]] bool isApplicationBlacklisted() const;

        /**
         * @brief Handles a change in the focused application.
         * @param activeApp The name of the newly focused application.
         */
        void onFocusChanged(const std::string& activeApp);

        /**
         * @brief Updates the status indicator in the system tray or terminal.
         * @param triggeredCommandName The name of the command that was triggered (optional).
         * @param triggeredCommandActive Whether the triggered command is now active (optional).
         */
        void updateStatusIndicator(const std::string& triggeredCommandName = "", std::optional<bool> triggeredCommandActive = std::nullopt);

        void setStatusCallback(StatusCallback callback);

        void startRecording(const SequenceConfig& config);
        void stopRecording();
        void pauseRecording();
        void resumeRecording();
        void discardRecording();
        [[nodiscard]] const RecordedSequence* getRecordedSequence() const;

    private:
        StatusCallback m_statusCallback{ nullptr };
        std::unique_ptr<IPlatformBackend> m_backend{ nullptr };
        std::unordered_map<Mouse, MouseHandler, HashFunction<Mouse>> m_mouseHandlers{};
        std::unordered_map<Key, KeyHandler, HashFunction<Key>> m_keyHandlers{};
        std::unordered_map<Key, SequenceHandler, HashFunction<Key>> m_sequenceHandlers{};
        std::unique_ptr<SequenceRecorder> m_recorder{ nullptr };
        ProgramArguments m_arguments{};
        std::vector<KeyInfo> m_keyInfo{};
        bool m_lastIsActiveIndicator{ false };
        std::string m_lastTriggeredCommandName;
        std::optional<bool> m_lastTriggeredCommandActive;
        std::unordered_set<int32_t> m_keysPressed{};
        std::unique_ptr<NotificationService> m_notificationService{ nullptr };
#ifdef AUTOINPUT_TESTING
        std::string m_testActiveApp;
#endif

        void startAutoClicker(InputHandler& handler);
    };

    /**
     * @brief Installs global input hooks.
     * @return True if successful.
     */
    bool installHooks();

    /**
     * @brief Runs the global event listener.
     */
    void runListener();

    /**
     * @brief Performs global cleanup.
     */
    void cleanup();

    inline Program* g_program{ nullptr };
}

#endif // INCLUDE_AUTOINPUT_AUTOINPUT_H