/**
 * @file windowManager.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_CORE_WINDOW_MANAGER_H
#define INCLUDE_AUTOINPUT_UI_CORE_WINDOW_MANAGER_H
#pragma once

#include "uiWindow.h"
#include <concepts>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace autoinput::ui
{
    /**
     * @brief Manages a collection of UI windows.
     * 
     * Handles window registration, lookup, and rendering.
     */
    class WindowManager
    {
    public:
        /**
         * @brief Registers a new window of type T.
         * @tparam T The window class (must inherit from UiWindow).
         * @tparam Args Constructor argument types.
         * @param id Unique identifier for the window.
         * @param args Arguments passed to the window's constructor.
         * @return Reference to the created window.
         */
        template <std::derived_from<UiWindow> T, typename... Args>
        T& addWindow(std::string id, Args&&... args);

        /**
         * @brief Finds a window and casts it to type T.
         * @tparam T The expected window type.
         * @param id The window identifier.
         * @return Pointer to the window of type T, or nullptr if not found or type mismatch.
         */
        template <std::derived_from<UiWindow> T>
        T* findAs(std::string_view id);

        /**
         * @brief Finds a window by ID.
         * @param id The window identifier.
         * @return Pointer to the window, or nullptr if not found.
         */
        UiWindow* find(std::string_view id);

        /**
         * @brief Opens the window with the given ID.
         * @param id The window identifier.
         */
        void open(std::string_view id);

        /**
         * @brief Opens the given window.
         * @param window The window to open.
         */
        void open(UiWindow* window);

        /**
         * @brief Renders all registered windows.
         */
        void render();

    private:
        std::unordered_map<std::string, std::unique_ptr<UiWindow>> m_windows;
    };

    template <std::derived_from<UiWindow> T, typename... Args>
    T& WindowManager::addWindow(std::string id, Args&&... args)
    {
        auto window = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *window;
        m_windows.emplace(std::move(id), std::move(window));
        return ref;
    }

    template <std::derived_from<UiWindow> T>
    T* WindowManager::findAs(std::string_view id)
    {
        return dynamic_cast<T*>(find(id));
    }
}

#endif // INCLUDE_AUTOINPUT_UI_CORE_WINDOW_MANAGER_H
