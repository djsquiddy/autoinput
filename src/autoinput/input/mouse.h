/**
 * @file mouse.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_INPUT_MOUSE_H
#define INCLUDE_AUTOINPUT_INPUT_MOUSE_H
#pragma once

#include <string>
#include <shared_mutex>
#include <mutex>
#include <utility>

#include "autoinput/support/types.h"
#include "autoinput/app/handlerState.h"

namespace autoinput
{
    struct MouseData;

    struct MouseInput
    {
        /**
         * @brief Constructs a MouseInput from platform-specific mouse data.
         * @param data Reference to the platform mouse data.
         */
        explicit MouseInput(MouseData& data);
        MouseData& data;

        /**
         * @brief Checks if the left button was pressed down.
         * @return True if left button down.
         */
        [[nodiscard]] bool isLeftButtonDown() const;

        /**
         * @brief Checks if the left button was released.
         * @return True if left button up.
         */
        [[nodiscard]] bool isLeftButtonUp() const;

        /**
         * @brief Checks if the right button was pressed down.
         * @return True if right button down.
         */
        [[nodiscard]] bool isRightButtonDown() const;

        /**
         * @brief Checks if the right button was released.
         * @return True if right button up.
         */
        [[nodiscard]] bool isRightButtonUp() const;

        /**
         * @brief Checks if the back button was pressed down.
         * @return True if back button down.
         */
        [[nodiscard]] bool isBackButtonDown() const;

        /**
         * @brief Checks if the back button was released.
         * @return True if back button up.
         */
        [[nodiscard]] bool isBackButtonUp() const;

        /**
         * @brief Checks if the forward button was released.
         * @return True if forward button up.
         */
        [[nodiscard]] bool isForwardButtonUp() const;

        /**
         * @brief Checks if the forward button was pressed down.
         * @return True if forward button down.
         */
        [[nodiscard]] bool isForwardButtonDown() const;

        /**
         * @brief Checks if the middle button was released.
         * @return True if middle button up.
         */
        [[nodiscard]] bool isMiddleButtonUp() const;

        /**
         * @brief Checks if the middle button was pressed down.
         * @return True if middle button down.
         */
        [[nodiscard]] bool isMiddleButtonDown() const;

        /**
         * @brief Checks if the event was synthetically generated.
         * @return True if synthetic.
         */
        [[nodiscard]] bool isSynthetic() const;

        /**
         * @brief Checks if the event is a mouse movement.
         * @return True if mouse move.
         */
        [[nodiscard]] bool isMouseMove() const;

        struct ButtonState
        {
            MouseButton button;
            bool isDown;
        };
        /**
         * @brief Gets the state of the button that triggered the event.
         * @return The ButtonState.
         */
        [[nodiscard]] ButtonState getButtonState() const;

        /**
         * @brief Prints information about the mouse input to the console.
         */
        void printInfo() const;
    };

    class MouseHandler : public InputHandler
    {
    public:
        /**
         * @brief Default constructor.
         */
        MouseHandler() = default;

        /**
         * @brief Virtual destructor.
         */
        ~MouseHandler() override {
            m_autoclickerThread.request_stop();
            m_cv.notify_all();
            if (m_autoclickerThread.joinable()) m_autoclickerThread.join();
        }

        /**
         * @brief Constructs a MouseHandler for a specific Mouse object.
         * @param mouse The mouse button and modifiers.
         * @param backend Pointer to the platform backend.
         */
        explicit MouseHandler(const Mouse mouse, IPlatformBackend* backend = nullptr) : InputHandler(backend), m_mouse{mouse} { }

        /**
         * @brief Constructs a MouseHandler for a specific MouseButton.
         * @param mouseButton The mouse button.
         * @param backend Pointer to the platform backend.
         */
        explicit MouseHandler(const MouseButton mouseButton, IPlatformBackend* backend = nullptr) : InputHandler(backend), m_mouse{mouseButton} { }

        /**
         * @brief Copy constructor.
         */
        MouseHandler(const MouseHandler& rhs);

        /**
         * @brief Move constructor.
         */
        MouseHandler(MouseHandler&& rhs) noexcept;

        /**
         * @brief Copy assignment operator.
         */
        MouseHandler& operator=(const MouseHandler& rhs);

        /**
         * @brief Move assignment operator.
         */
        MouseHandler& operator=(MouseHandler&& rhs) noexcept;

        /**
         * @brief Gets the name of the target mouse button.
         * @return The button name as a string.
         */
        [[nodiscard]] std::string getTargetName() const override
        {
            std::shared_lock lock(m_mouseMutex);
            return m_mouse.toString();
        }

        /**
         * @brief Gets the name of the button.
         * @return The button name.
         */
        [[nodiscard]] std::string getButtonName() const
        {
            std::shared_lock lock(m_mouseMutex);
            return m_mouse.toString();
        }

        /**
         * @brief Gets the Mouse object.
         * @return The Mouse object.
         */
        [[nodiscard]] Mouse getMouse() const
        {
            std::shared_lock lock(m_mouseMutex);
            return m_mouse;
        }

        /**
         * @brief Gets the MouseButton.
         * @return The MouseButton.
         */
        [[nodiscard]] MouseButton getMouseButton() const
        {
            std::shared_lock lock(m_mouseMutex);
            return m_mouse.button;
        }

        /**
         * @brief Toggles the press state of the mouse button.
         */
        void togglePressState() override;

        /**
         * @brief Equality operator.
         * @param rhs The other MouseHandler to compare with.
         * @return True if they handle the same mouse button.
         */
        bool operator==(const MouseHandler& rhs) const;

        /**
         * @brief Simulates pressing the mouse button down.
         */
        void press() override;

        /**
         * @brief Simulates releasing the mouse button.
         */
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
#endif // INCLUDE_AUTOINPUT_INPUT_MOUSE_H
