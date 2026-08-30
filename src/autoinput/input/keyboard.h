/**
 * @file keyboard.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_INPUT_KEYBOARD_H
#define INCLUDE_AUTOINPUT_INPUT_KEYBOARD_H
#pragma once

#include "autoinput/support/types.h"
#include "autoinput/app/handlerState.h"

#include <string>
#include <shared_mutex>
#include <mutex>
#include <cstdint>
#include <utility>
#include <functional>


namespace autoinput
{
    struct KeyboardData;

    struct KeyboardInput
    {
        /**
         * @brief Constructs a KeyboardInput from platform-specific keyboard data.
         * @param data Reference to the platform keyboard data.
         */
        explicit KeyboardInput(KeyboardData& data);
        KeyboardData& data;

        /**
         * @brief Checks if the event is a key down event.
         * @return True if key down.
         */
        [[nodiscard]] bool isKeyDown() const;

        /**
         * @brief Checks if the event is a key up event.
         * @return True if key up.
         */
        [[nodiscard]] bool isKeyUp() const;

        /**
         * @brief Checks if the event is a system key event.
         * @return True if system key.
         */
        [[nodiscard]] bool isSysKey() const;

        /**
         * @brief Checks if the event was synthetically generated.
         * @return True if synthetic.
         */
        [[nodiscard]] bool isSynthetic() const;

        /**
         * @brief Gets the character associated with the key event, if any.
         * @return The character, or 0 if none.
         */
        [[nodiscard]] int8_t getChar() const;

        /**
         * @brief Gets the function key number if the key is a function key (F1-F24).
         * @return The function key number, or 0 if not a function key.
         */
        [[nodiscard]] int64_t functionKey() const;

        /**
         * @brief Gets the general state of the key (Down, Up, etc.).
         * @return The KeyState.
         */
        [[nodiscard]] KeyState getKeyState() const;

        /**
         * @brief Prints information about the keyboard input to the console.
         */
        void printInfo() const;
    };

    class KeyHandler : public InputHandler
    {
    public:
        /**
         * @brief Default constructor.
         */
        KeyHandler() = default;

        /**
         * @brief Virtual destructor.
         */
        ~KeyHandler() override;

        /**
         * @brief Constructs a KeyHandler for a specific key.
         * @param key The key to handle.
         * @param backend Pointer to the platform backend.
         */
        explicit KeyHandler(Key key, IPlatformBackend* backend = nullptr) : InputHandler(backend), m_key{std::move(key)} {}

        /**
         * @brief Copy constructor.
         */
        KeyHandler(const KeyHandler& rhs);

        /**
         * @brief Move constructor.
         */
        KeyHandler(KeyHandler&& rhs) noexcept;

        /**
         * @brief Copy assignment operator.
         */
        KeyHandler& operator=(const KeyHandler& rhs);

        /**
         * @brief Move assignment operator.
         */
        KeyHandler& operator=(KeyHandler&& rhs) noexcept;

        /**
         * @brief Gets the name of the target key.
         * @return The key name as a string.
         */
        [[nodiscard]] std::string getTargetName() const override
        {
            std::shared_lock lock(m_keyMutex);
            return m_key.toString();
        }

        /**
         * @brief Gets the string representation of the key.
         * @return The key string.
         */
        [[nodiscard]] std::string getKeyValue() const
        {
            std::shared_lock lock(m_keyMutex);
            return m_key.toString();
        }

        /**
         * @brief Gets the Key object.
         * @return The Key object.
         */
        [[nodiscard]] Key getKey() const
        {
            std::shared_lock lock(m_keyMutex);
            return m_key;
        }

        /**
         * @brief Toggles the press state of the key.
         */
        void togglePressState() override;

        /**
         * @brief Equality operator.
         * @param rhs The other KeyHandler to compare with.
         * @return True if they handle the same key.
         */
        bool operator==(const KeyHandler& rhs) const;

        /**
         * @brief Simulates pressing the key down.
         */
        void press() override;

        /**
         * @brief Simulates releasing the key.
         */
        void release() override;

    private:
        Key m_key{};
        mutable std::shared_mutex m_keyMutex;
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


    inline KeyHandler::~KeyHandler()
    {
        m_autoclickerThread.request_stop();
        m_cv.notify_all();
        if (m_autoclickerThread.joinable())
        {
            m_autoclickerThread.join();
        }
    }

    inline KeyHandler::KeyHandler(const KeyHandler& rhs)
        : InputHandler(rhs)
    {
        std::shared_lock lock(rhs.m_keyMutex);
        m_key = rhs.m_key; // NOLINT(*-prefer-member-initializer)
    }

    inline KeyHandler::KeyHandler(KeyHandler&& rhs) noexcept
        : InputHandler(std::move(rhs))
    {
        std::unique_lock lock(rhs.m_keyMutex);
        m_key = std::move(rhs.m_key); // NOLINT(*-prefer-member-initializer)
    }

    inline KeyHandler& KeyHandler::operator=(KeyHandler&& rhs) noexcept
    {
        if (this == &rhs)
        {
            return *this;
        }

        std::unique_lock lock1(m_keyMutex, std::defer_lock);
        std::unique_lock lock2(rhs.m_keyMutex, std::defer_lock);
        std::lock(lock1, lock2);

        InputHandler::operator=(std::move(rhs));
        this->m_key = std::move(rhs.m_key);
        return *this;
    }

    inline KeyHandler& KeyHandler::operator=(const KeyHandler& rhs)
    {
        if (this == &rhs)
        {
            return *this;
        }

        std::unique_lock lock1(m_keyMutex, std::defer_lock);
        std::shared_lock lock2(rhs.m_keyMutex, std::defer_lock);
        std::lock(lock1, lock2);

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
        if (this == &rhs)
        {
            return true;
        }

        std::shared_lock lock1(m_keyMutex, std::defer_lock);
        std::shared_lock lock2(rhs.m_keyMutex, std::defer_lock);
        std::lock(lock1, lock2);
        return this->m_key == rhs.m_key;
    }
}
#endif // INCLUDE_AUTOINPUT_INPUT_KEYBOARD_H
