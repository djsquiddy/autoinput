/**
 * @file AutomationController.h
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_AUTOMATION_CONTROLLER_H
#define INCLUDE_AUTOINPUT_AUTOMATION_CONTROLLER_H
#pragma once


namespace autoinput
{
    class Program;
    class ProgramArguments;

    class AutomationController
    {
    public:
        bool start(const ProgramArguments& arguments);
        void stop();
        [[nodiscard]] bool running() const;

    private:
        std::unique_ptr<Program> m_program;
        std::jthread m_worker;
        std::atomic_bool m_running{ false };
    };
}

#endif // INCLUDE_AUTOINPUT_AUTOMATION_CONTROLLER_H
