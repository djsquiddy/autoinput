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
        [[nodiscard]] bool running() const;

        void setStatusCallback(StatusCallback callback);

    private:
        StatusCallback m_statusCallback{ nullptr };
        std::unique_ptr<Program> m_program;
        std::jthread m_worker;
        std::atomic_bool m_running{ false };
    };
}

#endif // INCLUDE_AUTOINPUT_AUTOMATION_CONTROLLER_H
