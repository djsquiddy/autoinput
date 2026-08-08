/**
 * @file modalWidgets.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_WIDGETS_MODAL_WIDGETS_H
#define INCLUDE_AUTOINPUT_UI_WIDGETS_MODAL_WIDGETS_H
#pragma once

#include <string>

namespace autoinput::ui::widgets
{
    /**
     * @brief Result of the save confirmation modal.
     */
    enum class SaveConfirmationResult
    {
        None,    /**< No action taken yet. */
        Save,    /**< User wants to save changes. */
        Discard, /**< User wants to discard changes. */
        Cancel   /**< User wants to cancel the close operation. */
    };

    /**
     * @brief Renders a modal dialog asking the user if they want to save changes.
     * 
     * @example
     * @code
     * auto result = RenderSaveConfirmationModal("Save Changes?", "Config file");
     * if (result == SaveConfirmationResult::Save) { save(); }
     * @endcode
     * 
     * @param title The title of the modal window.
     * @param name The name of the item being edited (shown in the message).
     * @return The result of the user's choice.
     */
    SaveConfirmationResult RenderSaveConfirmationModal(const std::string& title, const std::string& name);
}

#endif // INCLUDE_AUTOINPUT_UI_WIDGETS_MODAL_WIDGETS_H
