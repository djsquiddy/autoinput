/**
 * @file AutomationController.cpp
 * @author djsquiddy
 * @date August 2026
 */

#include "autoinput/app/automationController.h"
#include "autoinput/app/autoinput.h"
#include "autoinput/platform/backendFactory.h"
#include "autoinput/support/logger.h"
#include "autoinput/platform/platform.h"
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

    void AutomationController::stopCommand(std::string_view name)
    {
        if (m_program)
        {
            m_program->stopCommand(name);
        }
    }

    void AutomationController::pauseCommand(std::string_view name)
    {
        if (m_program)
        {
            m_program->pauseCommand(name);
        }
    }

    void AutomationController::resumeCommand(std::string_view name)
    {
        if (m_program)
        {
            m_program->resumeCommand(name);
        }
    }

    void AutomationController::togglePauseCommand(std::string_view name)
    {
        if (m_program)
        {
            m_program->togglePauseCommand(name);
        }
    }

    void AutomationController::stopAllCommands()
    {
        if (m_program)
        {
            m_program->stopAllCommands();
        }
    }

    void AutomationController::resume()
    {
        // TODO: implement
        m_paused = false;
    }

    void AutomationController::setStatusCallback(StatusCallback callback)
    {
        m_statusCallback = std::move(callback);
        if (m_program)
        {
            m_program->setStatusCallback(m_statusCallback);
        }
    }

    void AutomationController::pause()
    {
        // TODO: implement
        m_paused = true;
    }

    void AutomationController::runCommand(std::string_view name)
    {
        if (m_program && m_running)
        {
            m_program->runCommand(name);
        }
    }

    void AutomationController::startRecording(const SequenceConfig& config)
    {
        if (m_program)
        {
            m_program->startRecording(config);
        }
    }

    void AutomationController::stopRecording()
    {
        if (m_program)
        {
            m_program->stopRecording();
        }
    }

    void AutomationController::pauseRecording()
    {
        if (m_program)
        {
            m_program->pauseRecording();
        }
    }

    void AutomationController::resumeRecording()
    {
        if (m_program)
        {
            m_program->resumeRecording();
        }
    }

    void AutomationController::discardRecording()
    {
        if (m_program)
        {
            m_program->discardRecording();
        }
    }

    const RecordedSequence* AutomationController::getRecordedSequence() const
    {
        return m_program ? m_program->getRecordedSequence() : nullptr;
    }

    IPlatformBackend* AutomationController::getBackend() const
    {
        return m_program ? m_program->getBackend() : nullptr;
    }
}
