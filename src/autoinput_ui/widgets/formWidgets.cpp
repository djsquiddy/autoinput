/**
 * @file formWidgets.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "formWidgets.h"
#include "core/imguiScope.h"
#include "autoinput/waitDelay.h"
#include <format>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <array>

namespace autoinput::ui::widgets
{
    bool StringInput(const char* label, std::string& value)
    {
        return ImGui::InputText(label, &value);
    }

    bool StringListEditor(const char* label, std::vector<std::string>& values, const char* addButtonLabel)
    {
        bool changed = false;
        if (ImGui::TreeNode(label))
        {
            for (size_t i = 0; i < values.size(); ++i)
            {
                ImGuiIdScope scope{ i };
                if (ImGui::InputText("##item", &values[i]))
                {
                    changed = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Remove"))
                {
                    values.erase(values.begin() + static_cast<std::ptrdiff_t>(i));
                    changed = true;
                    break;
                }
            }

            if (ImGui::Button(addButtonLabel))
            {
                values.emplace_back("");
                changed = true;
            }
            ImGui::TreePop();
        }
        return changed;
    }

    bool StringCombo(const char* label, std::string& value, std::span<const std::string_view> options)
    {
        bool changed = false;
        if (ImGui::BeginCombo(label, value.c_str()))
        {
            for (const auto& option : options)
            {
                const bool isSelected = (value == option);
                if (ImGui::Selectable(std::string(option).c_str(), isSelected))
                {
                    value = std::string(option);
                    changed = true;
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        return changed;
    }
 
    bool StringCombo(const char* label, std::string& value, const std::vector<std::string>& options)
    {
        bool changed = false;
        if (ImGui::BeginCombo(label, value.c_str()))
        {
            for (const auto& option : options)
            {
                const bool isSelected = (value == option);
                if (ImGui::Selectable(option.c_str(), isSelected))
                {
                    value = option;
                    changed = true;
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        return changed;
    }

    bool StringVectorEditor(const char* label, std::vector<std::string>& items, const char* inputLabel, const char* addLabel)
    {
        bool changed = false;
        if (ImGui::TreeNode(label))
        {
            for (size_t i = 0; i < items.size(); ++i)
            {
                ImGuiIdScope scope{ i };
                if (ImGui::InputText(inputLabel, &items[i]))
                {
                    changed = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Remove"))
                {
                    items.erase(items.begin() + static_cast<std::ptrdiff_t>(i));
                    changed = true;
                    break;
                }
            }

            if (ImGui::Button(addLabel))
            {
                items.emplace_back("");
                changed = true;
            }
            ImGui::TreePop();
        }
        return changed;
    }

    bool WaitDurationEditor(const char* label, std::string& value)
    {
        bool changed = false;
        const auto parsedOpt = parseWaitDelayInput(value);
        WaitDelayInput parsed = parsedOpt.value_or(WaitDelayInput{ .hasValue = false, .durationType = "ms" });

        ImGui::PushID(label);

        ImGui::TextUnformatted(label);

        if (ImGui::Checkbox("Use range", &parsed.useRange))
        {
            parsed.hasValue = true;
            changed = true;
        }

        if (ImGui::InputDouble(parsed.useRange ? "Min" : "Value", &parsed.minValue, 0.0, 0.0, "%.3f"))
        {
            parsed.minValue = std::max(0.0, parsed.minValue);
            parsed.hasValue = true;
            changed = true;
        }

        if (parsed.useRange)
        {
            if (ImGui::InputDouble("Max", &parsed.maxValue, 0.0, 0.0, "%.3f"))
            {
                parsed.maxValue = std::max(parsed.minValue, parsed.maxValue);
                parsed.hasValue = true;
                changed = true;
            }
        }
        else
        {
            parsed.maxValue = parsed.minValue;
        }

        static constexpr std::array<std::string_view, 3> durationTypes = { "ms", "s", "m" };
        if (StringCombo("Duration Type", parsed.durationType, durationTypes))
        {
            parsed.hasValue = true;
            changed = true;
        }

        if (changed)
        {
            value = formatWaitDelayInput(parsed);
        }

        ImGui::TextDisabled("Serialized: %s", value.c_str());

        ImGui::PopID();

        return changed;
    }
}
