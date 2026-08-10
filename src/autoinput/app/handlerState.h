/**
 * @file handlerState.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_APP_HANDLERSTATE_H
#define INCLUDE_AUTOINPUT_APP_HANDLERSTATE_H
#pragma once

#include <string>
#include <atomic>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace autoinput
{
    class IPlatformBackend;

    class InputHandler
    {
    public:
        /**
         * @brief Constructs an InputHandler.
         * @param backend Pointer to the platform backend to use.
         */
        explicit InputHandler(IPlatformBackend* backend = nullptr) : m_backend(backend) {}

        /**
         * @brief Virtual destructor.
         */
        virtual ~InputHandler() {
            m_autoclickerThread.request_stop();
            m_cv.notify_all();
            if (m_autoclickerThread.joinable()) m_autoclickerThread.join();
        }

        /**
         * @brief Copy constructor.
         */
        InputHandler(const InputHandler& rhs);

        /**
         * @brief Move constructor.
         */
        InputHandler(InputHandler&& rhs) noexcept;

        /**
         * @brief Copy assignment operator.
         */
        InputHandler& operator=(const InputHandler& rhs);

        /**
         * @brief Move assignment operator.
         */
        InputHandler& operator=(InputHandler&& rhs) noexcept;

        /**
         * @brief Sets whether the handler is currently active (being listened for).
         * @param active True if active.
         */
        void setActive(const bool active) { m_isActive = active; }

        /**
         * @brief Gets whether the handler is currently active.
         * @return True if active.
         */
        [[nodiscard]] bool getActive() const { return m_isActive; }

        /**
         * @brief Sets whether the handler's action is currently paused.
         * @param paused True if paused.
         */
        void setPaused(const bool paused) { m_isPaused = paused; }

        /**
         * @brief Gets whether the handler's action is currently paused.
         * @return True if paused.
         */
        [[nodiscard]] bool getPaused() const { return m_isPaused; }

        /**
         * @brief Sets the display name for this handler.
         * @param name The display name.
         */
        void setName(const std::string& name) { m_name = name; }

        /**
         * @brief Gets the display name for this handler.
         * @return The display name if set, otherwise the target name.
         */
        [[nodiscard]] std::string getName() const { return m_name.empty() ? getTargetName() : m_name; }

        /**
         * @brief Sets the exclusive group for this handler.
         * @param group The group name.
         */
        void setExclusiveGroup(const std::string& group) { m_exclusiveGroup = group; }

        /**
         * @brief Gets the exclusive group for this handler.
         * @return The group name.
         */
        [[nodiscard]] std::string getExclusiveGroup() const { return m_exclusiveGroup; }

        /**
         * @brief Toggles the press state of the handler.
         */
        virtual void togglePressState() = 0;

        /**
         * @brief Triggers the press action.
         */
        virtual void press() = 0;

        /**
         * @brief Triggers the release action.
         */
        virtual void release() = 0;

        /**
         * @brief Gets the name of the target being handled (e.g. key or button name).
         * @return The target name.
         */
        [[nodiscard]] virtual std::string getTargetName() const = 0;

        /**
         * @brief Checks if the handler is currently in a pressed/active state.
         * @return True if pressed.
         */
        [[nodiscard]] bool isPressed() const { return m_isPressed.load(); }

    protected:
        IPlatformBackend* m_backend{ nullptr };
        std::atomic<bool> m_isActive{ false };
        std::atomic<bool> m_isPaused{ false };
        std::atomic<bool> m_isPressed{ false };
        std::string m_name;
        std::string m_exclusiveGroup;
        std::jthread m_autoclickerThread;
        std::condition_variable_any m_cv;
        std::mutex m_mutex;

        friend class Program;
    };

    inline InputHandler::InputHandler(const InputHandler& rhs)
        : m_backend(rhs.m_backend), m_name(rhs.m_name), m_exclusiveGroup(rhs.m_exclusiveGroup)
    {
        m_isPressed.store(rhs.m_isPressed.load());
        m_isActive.store(rhs.m_isActive.load());
        m_isPaused.store(rhs.m_isPaused.load());
        // jthread, cv, and mutex are not copyable
    }

    inline InputHandler::InputHandler(InputHandler&& rhs) noexcept
        : m_backend(rhs.m_backend), m_name(std::move(rhs.m_name)), m_exclusiveGroup(std::move(rhs.m_exclusiveGroup))
    {
        rhs.m_autoclickerThread.request_stop();
        if (rhs.m_autoclickerThread.joinable()) rhs.m_autoclickerThread.join();

        m_isPressed.store(rhs.m_isPressed.load());
        m_isActive.store(rhs.m_isActive.load());
        m_isPaused.store(rhs.m_isPaused.load());
        rhs.m_backend = nullptr;
    }

    inline InputHandler& InputHandler::operator=(const InputHandler& rhs)
    {
        if (this != &rhs)
        {
            m_autoclickerThread.request_stop();
            if (m_autoclickerThread.joinable()) m_autoclickerThread.join();

            m_backend = rhs.m_backend;
            m_name = rhs.m_name;
            m_exclusiveGroup = rhs.m_exclusiveGroup;
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
            m_autoclickerThread.request_stop();
            if (m_autoclickerThread.joinable()) m_autoclickerThread.join();

            rhs.m_autoclickerThread.request_stop();
            if (rhs.m_autoclickerThread.joinable()) rhs.m_autoclickerThread.join();

            m_backend = rhs.m_backend;
            m_name = std::move(rhs.m_name);
            m_exclusiveGroup = std::move(rhs.m_exclusiveGroup);
            m_isPressed.store(rhs.m_isPressed.load());
            m_isActive.store(rhs.m_isActive.load());
            m_isPaused.store(rhs.m_isPaused.load());
            rhs.m_backend = nullptr;
        }
        return *this;
    }
}

#endif // INCLUDE_AUTOINPUT_APP_HANDLERSTATE_H
