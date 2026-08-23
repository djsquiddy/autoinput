/**
 * @file commandEditor.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_EDITORS_COMMAND_EDITOR_H
#define INCLUDE_AUTOINPUT_UI_EDITORS_COMMAND_EDITOR_H
#pragma once

#include "../widgets/inputComboWidgets.h"

namespace autoinput
{
    struct CommandData;
}

namespace autoinput::ui::editors
{
    /**
     * @brief Which fields of a CommandData currently being edited are being captured/recorded.
     *
     * Grouping the capture indices into a single struct (instead of several adjacent `int&`
     * parameters) avoids an easily-swapped-by-mistake call signature.
     */
    struct CommandCaptureState
    {
        int startKeyIndex = -1;
        int controlIndex = -1;
        widgets::InputCaptureState inputs;
    };

    /**
     * @brief Renders an editor for a single CommandData object.
     * @param command The command data to edit.
     * @param capture Which of the command's start key/key/button entries is being captured.
     * @return true if any part of the command data was modified.
     */
    bool renderCommandEditor(CommandData& command, CommandCaptureState& capture);
}

#endif // INCLUDE_AUTOINPUT_UI_EDITORS_COMMAND_EDITOR_H
