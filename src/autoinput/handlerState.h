/**
 * @file handlerState.h
 * @author djsquiddy
 * @date July 2026
 */

#ifndef INCLUDE_AUTOINPUT_INPUT_HANDLER_H
#define INCLUDE_AUTOINPUT_INPUT_HANDLER_H
#pragma once

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

        virtual void togglePressState() = 0;
        virtual void press() = 0;
        virtual void release() = 0;
        [[nodiscard]] virtual std::string getName() const = 0;
        [[nodiscard]] bool isPressed() const { return m_isPressed; }

    protected:
        std::atomic<bool> m_isActive{ false };
        bool m_isPressed{ false };
        std::unique_ptr<std::thread> m_autoclickerThread{ nullptr };

        friend class Program;
    };

    inline InputHandler::InputHandler(const InputHandler& rhs): m_isPressed{ rhs.m_isPressed }
    {
        m_isActive.store(rhs.m_isActive.load());
    }

    inline InputHandler::InputHandler(InputHandler&& rhs) noexcept: m_isPressed{ rhs.m_isPressed }
    {
        m_isActive.store(rhs.m_isActive.load());
    }

    inline InputHandler& InputHandler::operator=(const InputHandler& rhs)
    {
        if (this != &rhs)
        {
            m_isPressed = rhs.m_isPressed;
            m_isActive.store(rhs.m_isActive.load());
        }
        return *this;
    }

    inline InputHandler& InputHandler::operator=(InputHandler&& rhs) noexcept
    {
        if (this != &rhs)
        {
            m_isPressed = rhs.m_isPressed;
            m_isActive.store(rhs.m_isActive.load());
        }
        return *this;
    }
}

#endif // INCLUDE_AUTOINPUT_INPUT_HANDLER_H
