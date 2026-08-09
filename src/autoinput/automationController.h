/**
 * @file AutomationController.h
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_AUTOMATION_CONTROLLER_H
#define INCLUDE_AUTOINPUT_AUTOMATION_CONTROLLER_H
#pragma once


#include "autoinput/autoinput.h"
#include <memory>
#include <thread>
#include <atomic>

namespace autoinput
{
    class ProgramArguments;

    class AutomationController
    {
    public:
        AutomationController();
        ~AutomationController();

        bool start(ProgramArguments arguments);
        void stop();
        [[nodiscard]] bool running() const { return m_running; }
        [[nodiscard]] bool paused() const { return m_paused; }

        void pause();
        void resume();

        void setStatusCallback(StatusCallback callback);

        void runCommand(std::string_view name);

        void startRecording(const SequenceConfig& config);
        void stopRecording();
        void pauseRecording();
        void resumeRecording();
        void discardRecording();
        [[nodiscard]] const RecordedSequence* getRecordedSequence() const;

        [[nodiscard]] IPlatformBackend* getBackend() const;

    private:
        StatusCallback m_statusCallback{ nullptr };
        std::unique_ptr<Program> m_program;
        std::jthread m_worker;
        std::atomic_bool m_running{ false };
        std::atomic_bool m_paused{ false };
    };
}

#endif // INCLUDE_AUTOINPUT_AUTOMATION_CONTROLLER_H
