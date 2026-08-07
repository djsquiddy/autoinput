/**
 * @file trayIcon.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "trayIcon.h"

namespace autoinput::tray
{
    bool MenuItem::isSeperator() const
    {
        return isFlagSet(state, MenuItemState::Seperator);
    }

    bool MenuItem::isEnabled() const
    {
        return isFlagSet(state, MenuItemState::Enabled);
    }

    bool MenuItem::isChecked() const
    {
        return isFlagSet(state, MenuItemState::Checked);
    }

    MenuItem createSeperatorMenuItem()
    {
        return {
            .text = "",
            .state = MenuItemState::Seperator | MenuItemState::Enabled
        };
    }

#ifndef _WIN32
    std::unique_ptr<ITrayIcon> createTrayIcon(void* nativeWindowHandle)
    {
        return nullptr;
    }
#endif
}
