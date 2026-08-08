/**
 * @file aboutWindow.h
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_ABOUT_WINDOW_H
#define INCLUDE_AUTOINPUT_ABOUT_WINDOW_H
#pragma once
#include "ui.h"

namespace autoinput::ui
{
    /**
     * @brief A simple about window showing application information.
     */
    class AboutWindow final : public UiWindow
    {
    public:
        /**
         * @brief Constructs a new AboutWindow.
         */
        AboutWindow()
            : UiWindow("About AutoInput")
        {
        }

    protected:
        /**
         * @brief Renders the about window content (version, author, etc.).
         */
        void renderContent() override;
    };
}

#endif // INCLUDE_AUTOINPUT_ABOUT_WINDOW_H
