/**
 * @file uiWindow.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_CORE_UI_WINDOW_H
#define INCLUDE_AUTOINPUT_UI_CORE_UI_WINDOW_H
#pragma once

#include <string>

namespace autoinput::ui
{
    /**
     * @brief Base class for all UI windows.
     * 
     * Handles common window logic such as title, open/close state, dirty state,
     * and save confirmation dialogs.
     */
    class UiWindow
    {
    public:
        /**
         * @brief Constructs a new UiWindow.
         * @param title The title of the window (used if titleKey is not provided or not found).
         * @param titleKey The localization key for the title.
         */
        explicit UiWindow(std::string title, std::string titleKey = "");
        
        virtual ~UiWindow() = default;

        /**
         * @brief Opens the window and sets focus.
         */
        void open();

        /**
         * @brief Sets whether the window should be fullscreen.
         * @param fullscreen true to make the window fullscreen.
         */
        void setFullscreen(bool fullscreen) { m_isFullscreen = fullscreen; }

        /**
         * @brief Checks if the window is fullscreen.
         * @return true if fullscreen, false otherwise.
         */
        [[nodiscard]] bool isFullscreen() const { return m_isFullscreen; }

        /**
         * @brief Requests the window to close.
         * 
         * If the window is dirty, it will show a save confirmation dialog.
         * Otherwise, it will close immediately.
         */
        void requestClose();

        /**
         * @brief Renders the window.
         * 
         * This should be called every frame. It handles the ImGui::Begin/End calls
         * and dispatches to protected hooks.
         */
        void render();

        /**
         * @brief Checks if the window is currently open.
         * @return true if open, false otherwise.
         */
        [[nodiscard]] bool isOpen() const;

        /**
         * @brief Checks if the window has unsaved changes.
         * @return true if dirty, false otherwise.
         */
        [[nodiscard]] bool isDirty() const;
        
        /**
         * @brief Gets the window title.
         * @return The window title string.
         */
        [[nodiscard]] const std::string& getTitle() const { return m_title; }

        /**
         * @brief Updates the window state.
         * 
         * This is called once per frame before rendering.
         */
        virtual void update() {}

    protected:
        /**
         * @brief Hook called when the window is opened.
         */
        virtual void onOpen() {}

        /**
         * @brief Hook called after a close has been confirmed (e.g. after saving/discarding).
         */
        virtual void onCloseConfirmed() {}

        /**
         * @brief Pure virtual hook for rendering the window's main content.
         */
        virtual void renderContent() = 0;

        /**
         * @brief Hook called to save changes.
         */
        virtual void save() {}

        /**
         * @brief Hook called to discard unsaved changes.
         */
        virtual void discardChanges();
        
        /**
         * @brief Renders the save confirmation modal if active.
         */
        virtual void renderSaveConfirmation();
        
        /**
         * @brief Gets ImGui window flags.
         * @return Bitmask of ImGuiWindowFlags.
         */
        [[nodiscard]] virtual int getFlags() const { return 0; }

        /**
         * @brief Checks if the window should have a close (X) button in the title bar.
         * @return true if it has a close button.
         */
        [[nodiscard]] virtual bool hasCloseButton() const { return true; }

        /**
         * @brief Marks the window as having unsaved changes.
         */
        void markDirty();

        /**
         * @brief Clears the dirty state.
         */
        void clearDirty();

    private:
        std::string m_title;
        std::string m_titleKey;
        bool m_isOpen{ false };
        bool m_isFullscreen{ false };
        bool m_shouldFocus{ false };
        bool m_isDirty{ false };
        bool m_showSaveConfirmation{ false };
    };
}

#endif // INCLUDE_AUTOINPUT_UI_CORE_UI_WINDOW_H
