/**
 * @file modalWidgets.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "modalWidgets.h"
#include <imgui.h>

namespace autoinput::ui::widgets
{
    SaveConfirmationResult RenderSaveConfirmationModal(const std::string& title, const std::string& name)
    {
        auto result = SaveConfirmationResult::None;

        const std::string modalTitle = title + "##" + name;
        ImGui::OpenPopup(modalTitle.c_str());

        if (ImGui::BeginPopupModal(modalTitle.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("You have unsaved changes in '%s'.\nDo you want to save them before closing?", name.c_str());
            ImGui::Separator();

            if (ImGui::Button("Save", ImVec2(120, 0)))
            {
                result = SaveConfirmationResult::Save;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Discard", ImVec2(120, 0)))
            {
                result = SaveConfirmationResult::Discard;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                result = SaveConfirmationResult::Cancel;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        
        return result;
    }
}
