/**
 * @file inputComboWidgets.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_WIDGETS_INPUT_COMBO_WIDGETS_H
#define INCLUDE_AUTOINPUT_UI_WIDGETS_INPUT_COMBO_WIDGETS_H
#pragma once

#include <string>
#include <vector>

namespace autoinput::ui::widgets
{
    /**
     * @brief Which entry of an `InputComboListEditor` (if any) is currently being recorded.
     *
     * Grouping the two indices into a single struct (instead of two adjacent `int&` parameters)
     * avoids an easily-swapped-by-mistake call signature while still letting callers persist the
     * capture target across frames (e.g. to drive a "Record shortcut" capture backend).
     */
    struct InputCaptureState
    {
        int keyIndex = -1;
        int buttonIndex = -1;
    };

    /**
     * @brief Renders a combined "Inputs" editor for keyboard keys and mouse buttons with modifiers.
     *
     * Presents entries from both vectors as removable chip rows (e.g. [Ctrl] [Shift] [F6] or
     * [Shift] [Mouse Left]), lets the user toggle Ctrl/Shift/Alt/Win modifiers per entry, and add
     * new keys or mouse buttons without needing to type raw config syntax. If `capture` points at
     * a valid entry, that entry is shown in "recording" state so callers can wire up a
     * "Record shortcut" capture backend.
     *
     * @example
     * @code
     * if (widgets::InputComboListEditor("Inputs", command.keys, command.buttons, capture))
     * {
     *     markDirty();
     * }
     * @endcode
     *
     * @param label Section label, e.g. "Inputs".
     * @param keys The vector of key combo strings (e.g. "ctrl+shift+f6").
     * @param buttons The vector of mouse button combo strings (e.g. "shift+left").
     * @param capture Which `keys`/`buttons` entry is currently being recorded, or -1/-1 for none.
     * @return true if `keys` or `buttons` were modified.
     */
    bool InputComboListEditor(
        const char* label,
        std::vector<std::string>& keys,
        std::vector<std::string>& buttons,
        InputCaptureState& capture);
}

#endif // INCLUDE_AUTOINPUT_UI_WIDGETS_INPUT_COMBO_WIDGETS_H
