//
// Created by djsquiddy on 3/9/2026.
//

#ifndef INCLUDE_AUTOINPUT_KEYBOARD_H
#define INCLUDE_AUTOINPUT_KEYBOARD_H
#pragma once
namespace autoinput
{
    struct KeyboardData;

    struct KeyboardInput
    {
        explicit KeyboardInput(KeyboardData& data);
        KeyboardData& data;

        [[nodiscard]] bool isKeyDown() const;
        [[nodiscard]] bool isKeyUp() const;
        [[nodiscard]] bool isSysKey() const;
        [[nodiscard]] int8_t getChar() const;
        [[nodiscard]] int64_t functionKey() const;

        void printInfo() const;
    };

    class KeyHandler
    {
    public:
        KeyHandler() = default;
        explicit KeyHandler(const Key& key) : m_key{ key } {}
        KeyHandler(const KeyHandler& rhs);
        KeyHandler(KeyHandler&& rhs) noexcept;
        KeyHandler& operator=(const KeyHandler& rhs);
        KeyHandler& operator=(KeyHandler&& rhs) noexcept;

        [[nodiscard]] std::string getKeyValue() const { return m_key.toString(); }

        void setActive(const bool active) { m_isActive = active; }
        [[nodiscard]] bool getActive() const { return m_isActive; }
        [[nodiscard]] Key getKey() const { return m_key; }

        void togglePressState();

        bool operator==(const KeyHandler& rhs) const;

        void pressKey();
        void releaseKey();

    private:
        friend class Program;
        Key m_key{};
        std::atomic<bool> m_isActive{ false };
        bool m_isPressed{ false };
        std::unique_ptr<std::thread> m_autoclickerThread{ nullptr };
    };

    template<>
    struct HashFunction<KeyHandler>
    {
        size_t operator()(const KeyHandler& handler) const
        {
            const auto [character, modifier] = handler.getKey();
            return std::hash<std::string>()(character) ^ std::hash<KeyModifier>()(modifier);
        }
    };

    inline KeyHandler::KeyHandler(const KeyHandler& rhs)
        : m_key{ rhs.m_key }
        , m_isPressed{ rhs.m_isPressed }
    {
        m_isActive.store(rhs.m_isActive.load());
    }

    inline KeyHandler::KeyHandler(KeyHandler&& rhs) noexcept
        : m_key{ std::move(rhs.m_key) }
        , m_isPressed{ rhs.m_isPressed }
    {
        m_isActive.store(rhs.m_isActive.load());
    }

    inline KeyHandler& KeyHandler::operator=(KeyHandler&& rhs) noexcept
    {
        if (this == &rhs)
        {
            return *this;
        }

        this->m_key = rhs.m_key;
        this->m_isPressed = rhs.m_isPressed;
        m_isActive.store(rhs.m_isActive.load());
        return *this;
    }

    inline KeyHandler& KeyHandler::operator=(const KeyHandler& rhs)
    {
        if (this == &rhs)
        {
            return *this;
        }

        this->m_key = rhs.m_key;
        this->m_isPressed = rhs.m_isPressed;
        m_isActive.store(rhs.m_isActive.load());
        return *this;
    }

    inline void KeyHandler::togglePressState()
    {
        if (m_isPressed)
        {
            releaseKey();
            return;
        }

        pressKey();
    }

    inline bool KeyHandler::operator==(const KeyHandler& rhs) const
    {
        return this->m_key == rhs.m_key;
    }
}
#endif // INCLUDE_AUTOINPUT_KEYBOARD_H