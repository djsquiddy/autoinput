/**
 * @file trayIcon.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_TRAY_ICON_H
#define INCLUDE_AUTOINPUT_TRAY_ICON_H
#pragma once

#include "autoinput/support/types.h"

#include <string>
#include <functional>
#include <vector>
#include <memory>

namespace autoinput::tray
{
    enum class MenuItemState
    {
        None = 0,
        Seperator = 1 << 0,
        Enabled = 1 << 1,
        Checked = 1 << 2
    };

    struct MenuItem
    {
        std::string text{};
        std::function<void()> action{ nullptr };
        MenuItemState state{ MenuItemState::None };
        std::vector<MenuItem> subItems{};

        [[nodiscard]] bool isSeperator() const;
        [[nodiscard]] bool isEnabled() const;
        [[nodiscard]] bool isChecked() const;
    };

    MenuItem createSeperatorMenuItem();

    class ITrayIcon
    {
    public:
        virtual ~ITrayIcon() = default;

        virtual void show() = 0;
        virtual void hide() = 0;
        virtual void setTooltip(const std::string& tooltip) = 0;
        virtual void setIcon(const std::string& iconResource) = 0;
        virtual void setMenu(const std::vector<MenuItem>& items) = 0;
        virtual void showNotification(const std::string& title, const std::string& message) = 0;
        virtual void update() = 0;
    };

    std::unique_ptr<ITrayIcon> createTrayIcon(void* nativeWindowHandle);
}

namespace autoinput
{
    AUTOINPUT_ENABLE_ENUM_BITWISE_OPERATORS(tray::MenuItemState);
}

#endif // INCLUDE_AUTOINPUT_TRAY_ICON_H
