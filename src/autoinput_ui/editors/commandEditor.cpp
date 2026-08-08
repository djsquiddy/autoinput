/**
 * @file commandEditor.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "commandEditor.h"
#include "../widgets/formWidgets.h"
#include "autoinput/config.h"
#include <imgui.h>
#include <array>

namespace autoinput::ui::editors
{
    namespace
    {
        constexpr std::array<std::string_view, 2> actionNames = { "click", "hold" };
    }

    bool renderCommandEditor(CommandData& command)
    {
        bool changed = false;

        if (widgets::StringInput("Name", command.name))
        {
            changed = true;
        }

        if (widgets::StringInput("Exclusive Group", command.exclusiveGroup))
        {
            changed = true;
        }

        if (widgets::StringCombo("Action", command.action, actionNames))
        {
            changed = true;
        }

        if (widgets::StringVectorEditor("Buttons", command.buttons, "##btn", "Add Button"))
        {
            changed = true;
        }

        if (widgets::StringVectorEditor("Keys", command.keys, "##key", "Add Key"))
        {
            changed = true;
        }

        if (widgets::StringVectorEditor("Start Keys", command.startKeys, "##start", "Add Start Key"))
        {
            changed = true;
        }

        ImGui::Text("Timing:");
        if (widgets::WaitDurationEditor("Press Wait", command.pressWait))
        {
            changed = true;
        }
        if (widgets::WaitDurationEditor("Release Wait", command.releaseWait))
        {
            changed = true;
        }

        return changed;
    }
}
