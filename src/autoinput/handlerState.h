/**
 * @file handlerState.h
 * @author djsquiddy
 * @date July 2026
 */

#ifndef INCLUDE_AUTOINPUT_INPUT_HANDLER_H
#define INCLUDE_AUTOINPUT_INPUT_HANDLER_H
#pragma once

#include <string>
#include <atomic>
#include <memory>
#include <thread>

namespace autoinput
{
    class InputHandler
    {
    public:
        InputHandler() = default;
        virtual ~InputHandler() = default;
        InputHandler(const InputHandler& rhs);
        InputHandler(InputHandler&& rhs) noexcept;
        InputHandler& operator=(const InputHandler& rhs);
        InputHandler& operator=(InputHandler&& rhs) noexcept;

        void setActive(const bool active) { m_isActive = active; }
        [[nodiscard]] bool getActive() const { return m_isActive; }

        void setPaused(const bool paused) { m_isPaused = paused; }
        [[nodiscard]] bool getPaused() const { return m_isPaused; }

        virtual void togglePressState() = 0;
        virtual void press() = 0;
        virtual void release() = 0;
        [[nodiscard]] virtual std::string getName() const = 0;
        [[nodiscard]] bool isPressed() const { return m_isPressed.load(); }

    protected:
        std::atomic<bool> m_isActive{ false };
        std::atomic<bool> m_isPaused{ false };
        std::atomic<bool> m_isPressed{ false };
        std::unique_ptr<std::thread> m_autoclickerThread{ nullptr };

        friend class Program;
    };

    inline InputHandler::InputHandler(const InputHandler& rhs)
    {
        m_isPressed.store(rhs.m_isPressed.load());
        m_isActive.store(rhs.m_isActive.load());
        m_isPaused.store(rhs.m_isPaused.load());
    }

    inline InputHandler::InputHandler(InputHandler&& rhs) noexcept
    {
        m_isPressed.store(rhs.m_isPressed.load());
        m_isActive.store(rhs.m_isActive.load());
        m_isPaused.store(rhs.m_isPaused.load());
    }

    inline InputHandler& InputHandler::operator=(const InputHandler& rhs)
    {
        if (this != &rhs)
        {
            m_isPressed.store(rhs.m_isPressed.load());
            m_isActive.store(rhs.m_isActive.load());
            m_isPaused.store(rhs.m_isPaused.load());
        }
        return *this;
    }

    inline InputHandler& InputHandler::operator=(InputHandler&& rhs) noexcept
    {
        if (this != &rhs)
        {
            m_isPressed.store(rhs.m_isPressed.load());
            m_isActive.store(rhs.m_isActive.load());
            m_isPaused.store(rhs.m_isPaused.load());
        }
        return *this;
    }
}

#endif // INCLUDE_AUTOINPUT_INPUT_HANDLER_H
