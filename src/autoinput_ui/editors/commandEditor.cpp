/**
 * @file commandEditor.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "commandEditor.h"
#include "../widgets/formWidgets.h"
#include "../widgets/inputComboWidgets.h"
#include "../core/localization.h"
#include "autoinput/config/config.h"
#include <imgui.h>
#include <array>

namespace autoinput::ui::editors
{
    namespace
    {
        constexpr std::array<std::string_view, 2> actionNames = { "click", "hold" };
        constexpr std::array<std::string_view, 9> controlActionNames = {
            "start", "toggle", "stop", "cancel", "pause", "resume", "toggle-pause", "stop-all", "exit"
        };
    }

    bool renderCommandEditor(CommandData& command, CommandCaptureState& capture)
    {
        bool changed = false;
        auto& loc = Localization::get();

        if (widgets::StringInput(loc.text("labels.name").data(), command.name))
        {
            changed = true;
        }

        if (widgets::StringInput(loc.text("labels.exclusiveGroup").data(), command.exclusiveGroup))
        {
            changed = true;
        }

        if (widgets::StringCombo(loc.text("labels.action").data(), command.action, actionNames))
        {
            changed = true;
        }

        if (widgets::InputComboListEditor(loc.text("labels.inputs").data(), command.keys, command.buttons, capture.inputs))
        {
            changed = true;
        }

        if (widgets::HotkeyVectorEditor(loc.text("labels.startKeys").data(), command.startKeys, "##start", loc.text("buttons.addStartKey").data(), capture.startKeyIndex))
        {
            changed = true;
        }

        ImGui::Text("%s", loc.text("labels.controls").data());
        int controlToDelete = -1;
        for (size_t i = 0; i < command.controls.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            auto& ctrl = command.controls[i];
            
            ImGui::SetNextItemWidth(120.0f);
            if (widgets::StringCombo("##ctrlAction", ctrl.action, controlActionNames))
            {
                changed = true;
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(140.0f);
            if (widgets::StringInput("##ctrlInput", ctrl.input))
            {
                changed = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("X"))
            {
                controlToDelete = static_cast<int>(i);
                changed = true;
            }
            ImGui::PopID();
        }

        if (controlToDelete >= 0 && controlToDelete < static_cast<int>(command.controls.size()))
        {
            command.controls.erase(command.controls.begin() + controlToDelete);
            changed = true;
        }

        if (ImGui::Button(loc.text("buttons.addControl").data()))
        {
            command.controls.push_back({ .action = "toggle", .input = "" });
            changed = true;
        }

        ImGui::Text("%s", loc.text("labels.timing").data());
        if (widgets::WaitDurationEditor(loc.text("labels.pressWait").data(), command.pressWait))
        {
            changed = true;
        }
        if (widgets::WaitDurationEditor(loc.text("labels.releaseWait").data(), command.releaseWait))
        {
            changed = true;
        }

        return changed;
    }
}
