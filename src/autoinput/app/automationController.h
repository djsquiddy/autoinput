/**
 * @file automationController.h
 * @brief Controller class that manages the automation runtime life-cycle.
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_APP_AUTOMATIONCONTROLLER_H
#define INCLUDE_AUTOINPUT_APP_AUTOMATIONCONTROLLER_H
#pragma once


#include "autoinput/app/autoinput.h"
#include <memory>
#include <thread>
#include <atomic>

namespace autoinput
{
    class ProgramArguments;

    /**
     * @brief Controller class that manages the execution of the autoinput Program in a background thread.
     */
    class AutomationController
    {
    public:
        /**
         * @brief Constructs an AutomationController.
         */
        AutomationController();
        
        /**
         * @brief Destructor that ensures the background thread is stopped.
         */
        ~AutomationController();

        /**
         * @brief Starts the automation with the given arguments.
         * @param arguments The program arguments to use.
         * @return True if started successfully.
         */
        bool start(ProgramArguments arguments);
        
        /**
         * @brief Stops the automation execution.
         */
        void stop();

        /**
         * @brief Stops a specific named command.
         * @param name The name of the command.
         */
        void stopCommand(std::string_view name);

        /**
         * @brief Pauses a specific named command.
         * @param name The name of the command.
         */
        void pauseCommand(std::string_view name);

        /**
         * @brief Resumes a specific named command.
         * @param name The name of the command.
         */
        void resumeCommand(std::string_view name);

        /**
         * @brief Toggles the pause state of a specific named command.
         * @param name The name of the command.
         */
        void togglePauseCommand(std::string_view name);

        /**
         * @brief Stops all active commands.
         */
        void stopAllCommands();
        
        /**
         * @brief Checks if the automation is currently running.
         * @return True if running.
         */
        [[nodiscard]] bool running() const { return m_running; }
        
        /**
         * @brief Checks if the automation is currently paused.
         * @return True if paused.
         */
        [[nodiscard]] bool paused() const { return m_paused; }

        /**
         * @brief Pauses the automation.
         */
        void pause();
        
        /**
         * @brief Resumes the automation.
         */
        void resume();

        /**
         * @brief Sets a callback for status updates.
         * @param callback The callback function.
         */
        void setStatusCallback(StatusCallback callback);

        /**
         * @brief Executes a command by name.
         * @param name The name of the command.
         */
        void runCommand(std::string_view name);

        /**
         * @brief Starts recording a sequence.
         * @param config The recording configuration.
         */
        void startRecording(const SequenceConfig& config);
        
        /**
         * @brief Stops the current recording and saves it.
         */
        void stopRecording();
        
        /**
         * @brief Pauses the current recording.
         */
        void pauseRecording();
        
        /**
         * @brief Resumes the current recording.
         */
        void resumeRecording();
        
        /**
         * @brief Discards the current recording.
         */
        void discardRecording();
        
        /**
         * @brief Gets the last recorded sequence.
         * @return Pointer to the RecordedSequence.
         */
        [[nodiscard]] const RecordedSequence* getRecordedSequence() const;

        /**
         * @brief Gets the platform backend used by the program.
         * @return Pointer to the IPlatformBackend.
         */
        [[nodiscard]] IPlatformBackend* getBackend() const;

    private:
        StatusCallback m_statusCallback{ nullptr };
        std::unique_ptr<Program> m_program;
        std::jthread m_worker;
        std::atomic_bool m_running{ false };
        std::atomic_bool m_paused{ false };
    };
}

#endif // INCLUDE_AUTOINPUT_APP_AUTOMATIONCONTROLLER_H
