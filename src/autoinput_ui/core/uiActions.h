/**
 * @file uiActions.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_CORE_UI_ACTIONS_H
#define INCLUDE_AUTOINPUT_UI_CORE_UI_ACTIONS_H
#pragma once

#include <string>
#include <functional>
#include <vector>
#include <string_view>

namespace autoinput::ui
{
    class WindowManager;

    /**
     * @brief Represents a common UI action.
     */
    struct UiAction
    {
        std::string id;
        std::string label;
        std::string category;
        std::string shortcut;
        std::string targetWindowId;
        std::function<void(WindowManager&)> callback;
    };

    /**
     * @brief Central registry for UI actions to avoid duplication.
     */
    class UiActionRegistry
    {
    public:
        /**
         * @brief Gets all registered UI actions.
         * @return A vector of UiActions.
         */
        static std::vector<UiAction> getActions();
        
        /**
         * @brief Executes an action by its ID.
         * @param id The action ID.
         * @param windowManager The window manager to use.
         * @return true if action was found and executed.
         */
        static bool execute(std::string_view id, WindowManager& windowManager);
    };
}

#endif // INCLUDE_AUTOINPUT_UI_CORE_UI_ACTIONS_H
