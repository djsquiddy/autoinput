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
            
            ImGui::SetNextItemWidth(110.0f);
            if (widgets::StringCombo("##ctrlAction", ctrl.action, controlActionNames))
            {
                changed = true;
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120.0f);
            if (widgets::StringInput("##ctrlInput", ctrl.input))
            {
                changed = true;
            }
            ImGui::SameLine();

            if (ImGui::Button("Preset..."))
            {
                ImGui::OpenPopup("PresetPopup");
            }

            if (ImGui::BeginPopup("PresetPopup"))
            {
                ImGui::TextDisabled("Wildcards");
                if (ImGui::MenuItem("mouse.all (Any Mouse Button)"))
                {
                    ctrl.input = "mouse.all";
                    changed = true;
                }
                if (ImGui::MenuItem("keys.all (Any Keyboard Key)"))
                {
                    ctrl.input = "keys.all";
                    changed = true;
                }
                if (ImGui::MenuItem("input.all (Any Input)"))
                {
                    ctrl.input = "input.all";
                    changed = true;
                }

                ImGui::Separator();
                ImGui::TextDisabled("Mouse Buttons");
                if (ImGui::MenuItem("mouse.back (Back / X1)"))
                {
                    ctrl.input = "mouse.back";
                    changed = true;
                }
                if (ImGui::MenuItem("mouse.forward (Forward / X2)"))
                {
                    ctrl.input = "mouse.forward";
                    changed = true;
                }
                if (ImGui::MenuItem("mouse.right (Right Click)"))
                {
                    ctrl.input = "mouse.right";
                    changed = true;
                }
                if (ImGui::MenuItem("mouse.left (Left Click)"))
                {
                    ctrl.input = "mouse.left";
                    changed = true;
                }
                if (ImGui::MenuItem("mouse.middle (Middle Click)"))
                {
                    ctrl.input = "mouse.middle";
                    changed = true;
                }

                ImGui::Separator();
                ImGui::TextDisabled("Common Keys");
                if (ImGui::MenuItem("space"))
                {
                    ctrl.input = "space";
                    changed = true;
                }
                if (ImGui::MenuItem("esc"))
                {
                    ctrl.input = "esc";
                    changed = true;
                }
                if (ImGui::MenuItem("f2"))
                {
                    ctrl.input = "f2";
                    changed = true;
                }
                if (ImGui::MenuItem("f6"))
                {
                    ctrl.input = "f6";
                    changed = true;
                }
                if (ImGui::MenuItem("f12"))
                {
                    ctrl.input = "f12";
                    changed = true;
                }

                ImGui::EndPopup();
            }

            ImGui::SameLine();
            if (capture.controlIndex == static_cast<int>(i))
            {
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "Press key/mouse...");
                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    capture.controlIndex = -1;
                }
            }
            else
            {
                if (ImGui::Button("Capture"))
                {
                    capture.controlIndex = static_cast<int>(i);
                }
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
            if (capture.controlIndex == controlToDelete)
            {
                capture.controlIndex = -1;
            }
            else if (capture.controlIndex > controlToDelete)
            {
                capture.controlIndex--;
            }
            changed = true;
        }

        if (ImGui::Button(loc.text("buttons.addControl").data()))
        {
            command.controls.push_back({ .action = "toggle", .input = "" });
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("+ Cancel (mouse.all)"))
        {
            command.controls.push_back({ .action = "cancel", .input = "mouse.all" });
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("+ Toggle (mouse.back)"))
        {
            command.controls.push_back({ .action = "toggle", .input = "mouse.back" });
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
