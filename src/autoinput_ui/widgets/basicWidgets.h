/**
 * @file basicWidgets.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_WIDGETS_BASIC_WIDGETS_H
#define INCLUDE_AUTOINPUT_UI_WIDGETS_BASIC_WIDGETS_H
#pragma once

#include <string>
#include <span>

namespace autoinput
{
    struct ValidationError;
}

namespace autoinput::ui::widgets
{
    /**
     * @brief Renders a status text message.
     * 
     * @example
     * @code
     * StatusText("Configuration loaded successfully.");
     * @endcode
     * 
     * @param message The message to display.
     */
    void StatusText(const std::string& message);

    /**
     * @brief Renders a list of validation errors.
     * 
     * @example
     * @code
     * auto errors = validator.validate(config);
     * ValidationErrors(errors);
     * @endcode
     * 
     * @param errors The errors to display.
     */
    void ValidationErrors(std::span<const ValidationError> errors);
    
    /**
     * @brief Executes a widget function and marks a boolean as dirty if the widget reports a change.
     * 
     * @example
     * @code
     * dirtyOnChange(m_isDirty, [&] {
     *     return ImGui::InputText("Application", &m_draft.application);
     * });
     * @endcode
     * 
     * @tparam Fn Widget function type.
     * @param dirty Reference to the dirty flag.
     * @param fn The widget function to execute.
     * @return true if the widget reported a change.
     */
    template <typename Fn>
    bool dirtyOnChange(bool& dirty, Fn&& fn)
    {
        if (fn())
        {
            dirty = true;
            return true;
        }
        return false;
    }
}

#endif // INCLUDE_AUTOINPUT_UI_WIDGETS_BASIC_WIDGETS_H
