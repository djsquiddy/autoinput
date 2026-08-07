/**
 * @file AutomationController.cpp
 * @author djsquiddy
 * @date August 2026
 */

#include "automationController.h"
#include "autoinput/autoinput.h"
#include "autoinput/backendFactory.h"
#include "autoinput/logger.h"
#include "autoinput/platform.h"
#include <future>

namespace autoinput
{
    AutomationController::AutomationController() = default;
    AutomationController::~AutomationController()
    {
        stop();
    }

    bool AutomationController::start(ProgramArguments arguments)
    {
        if (m_running)
        {
            return false;
        }

        m_program = std::make_unique<Program>();
        m_program->arguments() = std::move(arguments);

        auto backend = BackendFactory::createPlatformBackend();
        if (!backend)
        {
            return false;
        }

        m_program->setBackend(std::move(backend));

        if (m_statusCallback)
        {
            m_program->setStatusCallback(m_statusCallback);
        }

        m_running = true;
        std::promise<bool> startPromise;
        auto startFuture = startPromise.get_future();

        m_worker = std::jthread([this, promise = std::move(startPromise)](const std::stop_token& stoken) mutable
        {
            struct ScopedCleanup
            {
                AutomationController* controller;
                ~ScopedCleanup()
                {
                    g_program = nullptr;
                    controller->m_running = false;
                }
            } cleanup{this};

            // Set global program pointer because core functions depend on it.
            g_program = m_program.get();
            
            if (!m_program->init())
            {
                promise.set_value(false);
                return;
            }

            if (!m_program->installHooks())
            {
                promise.set_value(false);
                return;
            }

            promise.set_value(true);
            m_program->runListener();
            
            m_program->cleanup();
        });

        // Wait for the worker thread to report startup result
        try
        {
            if (!startFuture.get())
            {
                if (m_worker.joinable())
                {
                    m_worker.join();
                }
                m_running = false;
                return false;
            }
        }
        catch (...)
        {
            m_running = false;
            return false;
        }

        return true;
    }

    void AutomationController::stop()
    {
        if (!m_running)
        {
            return;
        }

        if (m_program)
        {
            m_program->requestStop();
        }
        
        platform::signalEnd();

        if (m_worker.joinable())
        {
            m_worker.request_stop();
            m_worker.join();
        }
        m_running = false;
    }

    void AutomationController::setStatusCallback(StatusCallback callback)
    {
        m_statusCallback = std::move(callback);
        if (m_program)
        {
            m_program->setStatusCallback(m_statusCallback);
        }
    }

    bool AutomationController::running() const
    {
        return m_running;
    }
}
