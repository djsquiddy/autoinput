/**
 * @file basicWidgets.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "basicWidgets.h"
#include "../core/localization.h"
#include "autoinput/config/configValidator.h"
#include <imgui.h>
#include <format>

namespace autoinput::ui::widgets
{
    void StatusText(const std::string& message)
    {
        ImGui::TextDisabled("%s", message.c_str());
    }

    void ValidationErrors(std::span<const ValidationError> errors)
    {
        if (errors.empty()) return;

        for (const auto& error : errors)
        {
            ImVec4 color;
            switch (error.severity)
            {
            case ValidationSeverity::Info: color = ImVec4(0.5f, 0.5f, 0.9f, 1.0f); break; // Blue-ish
            case ValidationSeverity::Warning: color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); break; // Yellow
            case ValidationSeverity::Error: color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); break; // Red
            default: color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break;
            }

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            std::string label = error.message;
            if (!error.section.empty())
            {
                if (!error.field.empty())
                    label = std::format("[{}.{}] {}", error.section, error.field, label);
                else
                    label = std::format("[{}] {}", error.section, label);
            }

            ImGui::BulletText("%s", label.c_str());
            
            if (!error.suggestedFix.empty())
            {
                ImGui::Indent();
                ImGui::TextDisabled("%s: %s", Localization::get().text("labels.suggestion").data(), error.suggestedFix.c_str());
                ImGui::Unindent();
            }
            ImGui::PopStyleColor();
        }
    }

    void RuntimeStatusIndicator(const std::string& status)
    {
        ImVec4 color(1.0f, 1.0f, 1.0f, 1.0f); // Default white
        std::string localizedStatus = status;
        auto& loc = Localization::get();

        if (status == "Running" || status == "Healthy")
        {
            color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
            localizedStatus = (status == "Running") ? loc.text("status.running") : loc.text("status.healthy");
        }
        else if (status == "Idle" || status == "Warning" || status == "Starting" || status == "Paused" || status == "Stopped")
        {
            color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
            if (status == "Idle") localizedStatus = loc.text("status.idle");
            else if (status == "Warning") localizedStatus = loc.text("status.warning");
            else if (status == "Starting") localizedStatus = loc.text("status.starting");
            else if (status == "Paused") localizedStatus = loc.text("status.paused");
            else if (status == "Stopped") localizedStatus = loc.text("status.stopped");
        }
        else if (status == "Error")
        {
            color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
            localizedStatus = loc.text("status.error");
        }
        else if (status == "Unknown")
        {
            localizedStatus = loc.text("status.unknown");
        }

        ImGui::TextColored(color, "%s", localizedStatus.c_str());
    }
}
