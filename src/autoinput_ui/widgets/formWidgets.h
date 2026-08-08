/**
 * @file formWidgets.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_WIDGETS_FORM_WIDGETS_H
#define INCLUDE_AUTOINPUT_UI_WIDGETS_FORM_WIDGETS_H
#pragma once

#include <string>
#include <vector>
#include <span>
#include <string_view>

namespace autoinput::ui::widgets
{
    /**
     * @brief A simple string input field.
     * 
     * @example
     * @code
     * if (StringInput("Username", m_username))
     * {
     *     markDirty();
     * }
     * @endcode
     * 
     * @param label The label for the input.
     * @param value The string value to edit.
     * @return true if the value was modified.
     */
    bool StringInput(const char* label, std::string& value);
    
    /**
     * @brief An editor for a list of strings with add/remove buttons.
     * 
     * @example
     * @code
     * if (StringListEditor("Search Paths", m_paths))
     * {
     *     markDirty();
     * }
     * @endcode
     * 
     * @param label The label for the list.
     * @param values The vector of strings to edit.
     * @param addButtonLabel The label for the "add" button.
     * @return true if the list was modified.
     */
    bool StringListEditor(
        const char* label,
        std::vector<std::string>& values,
        const char* addButtonLabel = "Add Item");

    /**
     * @brief A combo box for selecting a string from a list of options.
     * 
     * @example
     * @code
     * static const std::string_view options[] = { "Low", "Medium", "High" };
     * if (StringCombo("Priority", m_priority, options))
     * {
     *     markDirty();
     * }
     * @endcode
     * 
     * @param label The label for the combo box.
     * @param value The current selected string value.
     * @param options The list of available options.
     * @return true if the selection changed.
     */
    bool StringCombo(
        const char* label,
        std::string& value,
        std::span<const std::string_view> options);

    /**
     * @brief An editor for a vector of strings with a dedicated input field and add button.
     * 
     * @example
     * @code
     * if (StringVectorEditor("Tags", m_tags, "New Tag", "Add"))
     * {
     *     markDirty();
     * }
     * @endcode
     * 
     * @param label The label for the editor.
     * @param items The vector of strings to edit.
     * @param inputLabel The label for the item input field.
     * @param addLabel The label for the "add" button.
     * @return true if the items were modified.
     */
    bool StringVectorEditor(
        const char* label,
        std::vector<std::string>& items,
        const char* inputLabel,
        const char* addLabel);


    /**
     * @brief A wait-duration editor that supports either a single value or a range.
     *
     * The edited value is serialized as:
     * - "100ms"
     * - "100..250ms"
     * - "2s"
     * - "1..3s"
     *
     * @param label The visible label for the widget.
     * @param value The wait value string to edit.
     * @return true if the value was modified.
     */
    bool WaitDurationEditor(const char* label, std::string& value);
}

#endif // INCLUDE_AUTOINPUT_UI_WIDGETS_FORM_WIDGETS_H
