/**
 * @file keyboard.h
 * @author djsquiddy
 * @date March 2026
 */
#ifndef INCLUDE_AUTOINPUT_KEYBOARD_H
#define INCLUDE_AUTOINPUT_KEYBOARD_H
#pragma once

#include "types.h"
#include "handlerState.h"

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

        struct KeyState
        {
            int32_t keyCode{ INVALID_KEY };
            int32_t functionKey{ INVALID_KEY };
            int32_t virtualKey{ 0 };
            KeyModifier modifier{ KeyModifier::None };
        };
        [[nodiscard]] KeyState getKeyState() const;

        void printInfo() const;
    };

    class KeyHandler : public InputHandler
    {
    public:
        KeyHandler() = default;
        explicit KeyHandler(Key key) : m_key{std::move(key)} {}
        KeyHandler(const KeyHandler& rhs) = default;
        KeyHandler(KeyHandler&& rhs) noexcept;
        KeyHandler& operator=(const KeyHandler& rhs);
        KeyHandler& operator=(KeyHandler&& rhs) noexcept;

        [[nodiscard]] std::string getName() const override { return m_key.toString(); }
        [[nodiscard]] std::string getKeyValue() const { return m_key.toString(); }

        [[nodiscard]] Key getKey() const { return m_key; }

        void togglePressState() override;

        bool operator==(const KeyHandler& rhs) const;

        void press() override;
        void release() override;

    private:
        Key m_key{};
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

    inline KeyHandler::KeyHandler(KeyHandler&& rhs) noexcept
        : InputHandler(std::move(rhs))
        , m_key{ std::move(rhs.m_key) }
    {
    }

    inline KeyHandler& KeyHandler::operator=(KeyHandler&& rhs) noexcept
    {
        if (this == &rhs)
        {
            return *this;
        }

        InputHandler::operator=(std::move(rhs));
        this->m_key = rhs.m_key;
        return *this;
    }

    inline KeyHandler& KeyHandler::operator=(const KeyHandler& rhs)
    {
        if (this == &rhs)
        {
            return *this;
        }

        InputHandler::operator=(rhs);
        this->m_key = rhs.m_key;
        return *this;
    }

    inline void KeyHandler::togglePressState()
    {
        if (m_isPressed)
        {
            release();
            return;
        }

        press();
    }

    inline bool KeyHandler::operator==(const KeyHandler& rhs) const
    {
        return this->m_key == rhs.m_key;
    }
}
#endif // INCLUDE_AUTOINPUT_KEYBOARD_H