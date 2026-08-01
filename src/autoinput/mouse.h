/**
 * @file mouse.h
 * @author djsquiddy
 * @date March 2026
 */
#ifndef INCLUDE_AUTOINPUT_MOUSE_H
#define INCLUDE_AUTOINPUT_MOUSE_H
#pragma once

#include <string>
#include <shared_mutex>
#include <mutex>
#include <utility>

#include "autoinput/types.h"
#include "autoinput/handlerState.h"

namespace autoinput
{
    struct MouseData;

    struct MouseInput
    {
        explicit MouseInput(MouseData& data);
        MouseData& data;

        [[nodiscard]] bool isLeftButtonDown() const;
        [[nodiscard]] bool isLeftButtonUp() const;
        [[nodiscard]] bool isRightButtonDown() const;
        [[nodiscard]] bool isRightButtonUp() const;
        [[nodiscard]] bool isBackButtonDown() const;
        [[nodiscard]] bool isBackButtonUp() const;
        [[nodiscard]] bool isForwardButtonUp() const;
        [[nodiscard]] bool isForwardButtonDown() const;
        [[nodiscard]] bool isMiddleButtonUp() const;
        [[nodiscard]] bool isMiddleButtonDown() const;

        struct ButtonState
        {
            MouseButton button;
            bool isDown;
        };
        [[nodiscard]] ButtonState getButtonState() const;

        void printInfo() const;
    };

    class MouseHandler : public InputHandler
    {
    public:
        MouseHandler() = default;
        explicit MouseHandler(const Mouse mouse) : m_mouse{mouse} { }
        explicit MouseHandler(const MouseButton mouseButton) : m_mouse{mouseButton} { }
        MouseHandler(const MouseHandler& rhs);
        MouseHandler(MouseHandler&& rhs) noexcept;
        MouseHandler& operator=(const MouseHandler& rhs);
        MouseHandler& operator=(MouseHandler&& rhs) noexcept;

        [[nodiscard]] std::string getName() const override
        {
            std::shared_lock lock(m_mouseMutex);
            return m_mouse.toString();
        }

        [[nodiscard]] std::string getButtonName() const
        {
            std::shared_lock lock(m_mouseMutex);
            return m_mouse.toString();
        }

        [[nodiscard]] Mouse getMouse() const
        {
            std::shared_lock lock(m_mouseMutex);
            return m_mouse;
        }

        [[nodiscard]] MouseButton getMouseButton() const
        {
            std::shared_lock lock(m_mouseMutex);
            return m_mouse.button;
        }

        void togglePressState() override;

        bool operator==(const MouseHandler& rhs) const;

        void press() override;
        void release() override;

    private:
        Mouse m_mouse{};
        mutable std::shared_mutex m_mouseMutex;
    };

    template<>
    struct HashFunction<MouseHandler>
    {
        size_t operator()(const MouseHandler& handler) const
        {
            return HashFunction<Mouse>()(handler.getMouse());
        }
    };

    inline MouseHandler::MouseHandler(const MouseHandler& rhs)
        : InputHandler(rhs)
    {
        std::shared_lock lock(rhs.m_mouseMutex);
        m_mouse = rhs.m_mouse;
    }

    inline MouseHandler::MouseHandler(MouseHandler&& rhs) noexcept
        : InputHandler(std::move(rhs))
    {
        std::unique_lock lock(rhs.m_mouseMutex);
        m_mouse = std::move(rhs.m_mouse);
    }

    inline MouseHandler& MouseHandler::operator=(MouseHandler&& rhs) noexcept
    {
        if (this == &rhs)
        {
            return *this;
        }

        std::unique_lock lock1(m_mouseMutex, std::defer_lock);
        std::unique_lock lock2(rhs.m_mouseMutex, std::defer_lock);
        std::lock(lock1, lock2);

        InputHandler::operator=(std::move(rhs));
        this->m_mouse = std::move(rhs.m_mouse);
        return *this;
    }

    inline MouseHandler& MouseHandler::operator=(const MouseHandler& rhs)
    {
        if (this == &rhs)
        {
            return *this;
        }

        std::unique_lock lock1(m_mouseMutex, std::defer_lock);
        std::shared_lock lock2(rhs.m_mouseMutex, std::defer_lock);
        std::lock(lock1, lock2);

        InputHandler::operator=(rhs);
        this->m_mouse = rhs.m_mouse;
        return *this;
    }

    inline void MouseHandler::togglePressState()
    {
        if (m_isPressed)
        {
            release();
            return;
        }

        press();
    }

    inline bool MouseHandler::operator==(const MouseHandler& rhs) const
    {
        if (this == &rhs)
        {
            return true;
        }

        std::shared_lock lock1(m_mouseMutex, std::defer_lock);
        std::shared_lock lock2(rhs.m_mouseMutex, std::defer_lock);
        std::lock(lock1, lock2);
        return this->m_mouse == rhs.m_mouse;
    }
}
#endif // INCLUDE_AUTOINPUT_MOUSE_H
