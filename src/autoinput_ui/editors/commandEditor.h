/**
 * @file commandEditor.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_EDITORS_COMMAND_EDITOR_H
#define INCLUDE_AUTOINPUT_UI_EDITORS_COMMAND_EDITOR_H
#pragma once

namespace autoinput
{
    struct CommandData;
}

namespace autoinput::ui::editors
{
    /**
     * @brief Renders an editor for a single CommandData object.
     * @param command The command data to edit.
     * @return true if any part of the command data was modified.
     */
    bool renderCommandEditor(CommandData& command);
}

#endif // INCLUDE_AUTOINPUT_UI_EDITORS_COMMAND_EDITOR_H
