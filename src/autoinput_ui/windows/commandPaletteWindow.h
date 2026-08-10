/**
 * @file commandPaletteWindow.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_WINDOWS_COMMAND_PALETTE_WINDOW_H
#define INCLUDE_AUTOINPUT_UI_WINDOWS_COMMAND_PALETTE_WINDOW_H
#pragma once

#include "../core/uiWindow.h"
#include <string>
#include <vector>
#include <functional>

namespace autoinput::ui
{
    class WindowManager;

    /**
     * @brief A command palette for quick access to windows and actions.
     */
    struct PaletteCommand
    {
        std::string label;
        std::string category;
        std::function<void()> action;
    };

    class CommandPaletteWindow : public UiWindow
    {
    public:
        explicit CommandPaletteWindow(WindowManager& windowManager);

    protected:
        void renderContent() override;
        void onOpen() override;
        int getFlags() const override;
        bool hasCloseButton() const override { return false; }

    private:
        void refreshActions();
        void executeCommand(const PaletteCommand& cmd);

        WindowManager& m_windowManager;
        std::vector<PaletteCommand> m_commands;
        char m_filter[128] = "";
        int m_selectedIndex = 0;
        bool m_focusSearch = false;
    };
}

#endif // INCLUDE_AUTOINPUT_UI_WINDOWS_COMMAND_PALETTE_WINDOW_H
