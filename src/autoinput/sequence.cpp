/**
 * @file sequence.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/sequence.h"
#include "autoinput/backend.h"
#include "autoinput/logger.h"
#include "autoinput/waitDelay.h"
#include <thread>

namespace autoinput
{
    SequenceHandler::SequenceHandler(RecordedSequence sequence, IPlatformBackend* backend)
        : InputHandler(backend), m_sequence(std::move(sequence))
    {
        m_name = m_sequence.name;
    }

    SequenceHandler::SequenceHandler(const SequenceHandler& rhs)
        : InputHandler(rhs), m_sequence(rhs.m_sequence)
    {
    }

    SequenceHandler::SequenceHandler(SequenceHandler&& rhs) noexcept
        : InputHandler(std::move(rhs)), m_sequence(std::move(rhs.m_sequence))
    {
    }

    SequenceHandler& SequenceHandler::operator=(const SequenceHandler& rhs)
    {
        if (this != &rhs)
        {
            InputHandler::operator=(rhs);
            m_sequence = rhs.m_sequence;
        }
        return *this;
    }

    SequenceHandler& SequenceHandler::operator=(SequenceHandler&& rhs) noexcept
    {
        if (this != &rhs)
        {
            InputHandler::operator=(std::move(rhs));
            m_sequence = std::move(rhs.m_sequence);
        }
        return *this;
    }

    void SequenceHandler::togglePressState()
    {
        if (m_isActive)
        {
            release();
        }
        else
        {
            press();
        }
    }

    void SequenceHandler::press()
    {
        if (!m_backend) return;
        if (bool expected = false; m_isActive.compare_exchange_strong(expected, true))
        {
            m_autoclickerThread = std::make_unique<std::thread>(&SequenceHandler::playback, this);
        }
    }

    void SequenceHandler::release()
    {
        m_isActive = false;
        if (m_autoclickerThread && m_autoclickerThread->joinable())
        {
            m_autoclickerThread->detach(); // We don't want to block the hook thread
            m_autoclickerThread.reset();
        }
    }

    void SequenceHandler::playback()
    {
        Logger::info("Starting playback of sequence: {}\n", m_sequence.name);
        
        do
        {
            for (const auto& event : m_sequence.events)
            {
                if (!m_isActive) break;

                auto delay = parseWaitDelay(event.delay);
                if (delay.count() > 0)
                {
                    std::this_thread::sleep_for(delay);
                }

                if (!m_isActive) break;

                switch (event.type)
                {
                case RecordedEventType::KeyDown:
                    if (event.key) m_backend->keyDown(Key::fromString(*event.key));
                    break;
                case RecordedEventType::KeyUp:
                    if (event.key) m_backend->keyUp(Key::fromString(*event.key));
                    break;
                case RecordedEventType::MouseDown:
                    if (event.button) m_backend->mouseDown(Mouse::fromString(*event.button));
                    break;
                case RecordedEventType::MouseUp:
                    if (event.button) m_backend->mouseUp(Mouse::fromString(*event.button));
                    break;
                case RecordedEventType::MouseMove:
                    if (event.x && event.y) m_backend->moveMouseTo(*event.x, *event.y);
                    break;
                default:
                    break;
                }
            }
        } while (m_isActive && m_sequence.repeat);

        m_isActive = false;
        Logger::info("Finished playback of sequence: {}\n", m_sequence.name);
    }
}
