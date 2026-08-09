/**
 * @file windowManager.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "windowManager.h"
#include <ranges>

namespace autoinput::ui
{
    UiWindow* WindowManager::find(std::string_view id)
    {
        const std::string windowId{ id };
        return m_windows.contains(windowId) ? m_windows[windowId].get() : nullptr;
    }

    void WindowManager::open(std::string_view id)
    {
        if (auto* window = find(id))
        {
            window->open();
        }
    }

    void WindowManager::render()
    {
        for (auto& window : m_windows | std::views::values)
        {
            if (window->isOpen())
            {
                window->update();
                window->render();
            }
        }
    }
}
