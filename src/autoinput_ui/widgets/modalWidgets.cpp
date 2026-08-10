/**
 * @file modalWidgets.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "modalWidgets.h"
#include "../core/localization.h"
#include <imgui.h>

namespace autoinput::ui::widgets
{
    SaveConfirmationResult RenderSaveConfirmationModal(const std::string& title, const std::string& name)
    {
        auto result = SaveConfirmationResult::None;
        auto& loc = Localization::get();

        const std::string modalTitle = (title.empty() ? std::string(loc.text("modals.saveConfirmationTitle")) : title) + "##" + name;
        ImGui::OpenPopup(modalTitle.c_str());

        if (ImGui::BeginPopupModal(modalTitle.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("%s", loc.format("modals.saveConfirmationMessage", name).c_str());
            ImGui::Separator();

            if (ImGui::Button(loc.text("buttons.save").data(), ImVec2(120, 0)))
            {
                result = SaveConfirmationResult::Save;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(loc.text("buttons.discard").data(), ImVec2(120, 0)))
            {
                result = SaveConfirmationResult::Discard;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(loc.text("buttons.cancel").data(), ImVec2(120, 0)))
            {
                result = SaveConfirmationResult::Cancel;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        
        return result;
    }
}
