/**
 * @file mouse.h
 * @author djsquiddy
 * @date March 2026
 */
#ifndef INCLUDE_AUTOINPUT_MOUSE_H
#define INCLUDE_AUTOINPUT_MOUSE_H
#pragma once

#include "types.h"
#include "handlerState.h"

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
        explicit MouseHandler(const MouseButton mouseButton) : m_mouseButton{mouseButton} { }
        MouseHandler(const MouseHandler& rhs) = default;
        MouseHandler(MouseHandler&& rhs) noexcept;
        MouseHandler& operator=(const MouseHandler& rhs);
        MouseHandler& operator=(MouseHandler&& rhs) noexcept;

        [[nodiscard]] std::string getName() const override { return mouseButtonToString(m_mouseButton); }
        [[nodiscard]] std::string getButtonName() const { return mouseButtonToString(m_mouseButton); }

        [[nodiscard]] MouseButton getMouseButton() const { return m_mouseButton; }

        void togglePressState() override;

        bool operator==(const MouseHandler& rhs) const;

        void press() override;
        void release() override;

    private:
        MouseButton m_mouseButton{ MouseButton::NONE };
    };

    template<>
    struct HashFunction<MouseHandler>
    {
        size_t operator()(const MouseHandler& handler) const
        {
            using button_t = std::underlying_type_t<MouseButton>;
            return std::hash<button_t>()(static_cast<button_t>(handler.getMouseButton()));
        }
    };

    inline MouseHandler::MouseHandler(MouseHandler&& rhs) noexcept
        : InputHandler(std::move(rhs))
        , m_mouseButton{ rhs.m_mouseButton }
    {
    }

    inline MouseHandler& MouseHandler::operator=(MouseHandler&& rhs) noexcept
    {
        if (this == &rhs)
        {
            return *this;
        }

        InputHandler::operator=(std::move(rhs));
        this->m_mouseButton = rhs.m_mouseButton;
        return *this;
    }

    inline MouseHandler& MouseHandler::operator=(const MouseHandler& rhs)
    {
        if (this == &rhs)
        {
            return *this;
        }

        InputHandler::operator=(rhs);
        this->m_mouseButton = rhs.m_mouseButton;
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
        return this->m_mouseButton == rhs.m_mouseButton;
    }
}
#endif // INCLUDE_AUTOINPUT_MOUSE_H
