/**
 * @file basicWidgets.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "basicWidgets.h"
#include "autoinput/configValidator.h"
#include <imgui.h>

namespace autoinput::ui::widgets
{
    void StatusText(const std::string& message)
    {
        ImGui::TextDisabled("%s", message.c_str());
    }

    void ValidationErrors(std::span<const ValidationError> errors)
    {
        if (errors.empty()) return;

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        ImGui::Text("Validation Errors:");
        for (const auto& error : errors)
        {
            ImGui::BulletText("%s", error.message.c_str());
        }
        ImGui::PopStyleColor();
    }
}
