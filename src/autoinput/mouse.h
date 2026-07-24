//
// Created by djsqu on 3/9/2026.
//

#ifndef INCLUDE_AUTOINPUT_MOUSE_H
#define INCLUDE_AUTOINPUT_MOUSE_H
#pragma once


namespace autoinput
{
    enum class MouseButton : uint8_t;

    class MouseHandler
    {
    public:
        MouseHandler() = default;
        explicit MouseHandler(const MouseButton mouseButton) : m_mouseButton{mouseButton} { }
        MouseHandler(const MouseHandler& rhs);
        MouseHandler(MouseHandler&& rhs) noexcept;
        MouseHandler& operator=(const MouseHandler& rhs);
        MouseHandler& operator=(MouseHandler&& rhs) noexcept;

        [[nodiscard]] std::string getButtonName() const { return mouseButtonToString(m_mouseButton); }

        void setActive(const bool isActive) { m_isActive = isActive; }
        [[nodiscard]] bool getActive() const { return m_isActive; }
        [[nodiscard]] MouseButton getMouseButton() const { return m_mouseButton; }

        void togglePressState();

        bool operator==(const MouseHandler& rhs) const;

        void pressButton();
        void releaseButton();

    private:
        friend class Program;
        MouseButton m_mouseButton{ MouseButton::NONE };
        std::atomic<bool> m_isActive{ false };
        bool m_isButtonPressed{ false };
        std::unique_ptr<std::thread> m_autoclickerThread{ nullptr };
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

    inline MouseHandler::MouseHandler(const MouseHandler& rhs)
        : m_mouseButton{ rhs.m_mouseButton }
        , m_isButtonPressed{ rhs.m_isButtonPressed }
    {
        m_isActive.store(rhs.m_isActive.load());
    }

    inline MouseHandler::MouseHandler(MouseHandler&& rhs) noexcept
        : m_mouseButton{ rhs.m_mouseButton }
          , m_isButtonPressed{ rhs.m_isButtonPressed }
    {
        m_isActive.store(rhs.m_isActive.load());
    }

    inline MouseHandler& MouseHandler::operator=(MouseHandler&& rhs) noexcept
    {
        if (this == &rhs)
        {
            return *this;
        }

        this->m_mouseButton = rhs.m_mouseButton;
        this->m_isButtonPressed = rhs.m_isButtonPressed;
        m_isActive.store(rhs.m_isActive.load());
        return *this;
    }

    inline MouseHandler& MouseHandler::operator=(const MouseHandler& rhs)
    {
        if (this == &rhs)
        {
            return *this;
        }

        this->m_mouseButton = rhs.m_mouseButton;
        this->m_isButtonPressed = rhs.m_isButtonPressed;
        m_isActive.store(rhs.m_isActive.load());
        return *this;
    }

    inline void MouseHandler::togglePressState()
    {
        if (m_isButtonPressed)
        {
            releaseButton();
            return;
        }

        pressButton();
    }

    inline bool MouseHandler::operator==(const MouseHandler& rhs) const
    {
        return this->m_mouseButton == rhs.m_mouseButton;
    }
}
#endif // INCLUDE_AUTOINPUT_MOUSE_H