/**
 * @file inputComboWidgets.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "inputComboWidgets.h"
#include "core/imguiScope.h"
#include "core/localization.h"
#include "autoinput/support/types.h"
#include "autoinput/support/utils.h"
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <array>
#include <cctype>
#include <unordered_map>
#include <utility>

namespace autoinput::ui::widgets
{
    namespace
    {
        constexpr std::array<std::pair<std::string_view, std::string_view>, 5> knownMouseButtons = { {
            { "left", "labels.mouseButtonLeft" },
            { "right", "labels.mouseButtonRight" },
            { "middle", "labels.mouseButtonMiddle" },
            { "back", "labels.mouseButtonBack" },
            { "forward", "labels.mouseButtonForward" },
        } };

        // Per-widget-instance scratch text buffers for the "Add key" input, keyed by ImGui ID
        // so multiple InputComboListEditor instances (e.g. one per command) don't share state.
        std::unordered_map<ImGuiID, std::string>& newKeyBuffers() // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
        {
            static std::unordered_map<ImGuiID, std::string> buffers;
            return buffers;
        }

        std::string keyBaseDisplayName(const Key& key)
        {
            if (isFlagSet(key.modifier, KeyModifier::Function))
            {
                return "F" + key.character;
            }
            if (key.character.size() == 1)
            {
                return { 1, static_cast<char>(std::toupper(static_cast<unsigned char>(key.character.at(0)))) };
            }
            return capitalize(key.character);
        }

        std::string buttonBaseDisplayName(const Mouse& mouse)
        {
            return std::string(Localization::get().text("buttons.mouse")) + " " + capitalize(mouseButtonToString(mouse.button));
        }

        std::vector<std::string> modifierChips(const KeyModifier modifier)
        {
            auto& loc = Localization::get();
            std::vector<std::string> chips;
            if (isFlagSet(modifier, KeyModifier::Ctrl)) { chips.emplace_back(loc.text("labels.modifierCtrl")); }
            if (isFlagSet(modifier, KeyModifier::Shift)) { chips.emplace_back(loc.text("labels.modifierShift")); }
            if (isFlagSet(modifier, KeyModifier::Alt)) { chips.emplace_back(loc.text("labels.modifierAlt")); }
            if (isFlagSet(modifier, KeyModifier::Meta)) { chips.emplace_back(loc.text("labels.modifierWin")); }
            return chips;
        }

        void renderChips(const std::vector<std::string>& chips)
        {
            for (size_t i = 0; i < chips.size(); ++i)
            {
                if (i > 0)
                {
                    ImGui::SameLine(0.0f, 4.0f);
                    ImGui::TextDisabled("+");
                    ImGui::SameLine(0.0f, 4.0f);
                }
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.24f, 0.35f, 0.55f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.35f, 0.55f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.24f, 0.35f, 0.55f, 1.0f));
                ImGui::SmallButton(chips[i].c_str());
                ImGui::PopStyleColor(3);
            }
        }

        // Toggles the Ctrl/Shift/Alt/Win bits of `modifier` in-place, preserving any other
        // bits (e.g. the internal Function flag used for F-keys). Returns true if changed.
        bool modifierCheckboxes(KeyModifier& modifier)
        {
            auto& loc = Localization::get();
            const std::string ctrlLabel(loc.text("labels.modifierCtrl"));
            const std::string shiftLabel(loc.text("labels.modifierShift"));
            const std::string altLabel(loc.text("labels.modifierAlt"));
            const std::string winLabel(loc.text("labels.modifierWin"));

            bool ctrl = isFlagSet(modifier, KeyModifier::Ctrl);
            bool shift = isFlagSet(modifier, KeyModifier::Shift);
            bool alt = isFlagSet(modifier, KeyModifier::Alt);
            bool win = isFlagSet(modifier, KeyModifier::Meta);

            bool changed = false;
            if (ImGui::Checkbox(ctrlLabel.c_str(), &ctrl)) { changed = true; }
            ImGui::SameLine();
            if (ImGui::Checkbox(shiftLabel.c_str(), &shift)) { changed = true; }
            ImGui::SameLine();
            if (ImGui::Checkbox(altLabel.c_str(), &alt)) { changed = true; }
            ImGui::SameLine();
            if (ImGui::Checkbox(winLabel.c_str(), &win)) { changed = true; }

            if (changed)
            {
                constexpr KeyModifier togglable = KeyModifier::Ctrl | KeyModifier::Shift | KeyModifier::Alt | KeyModifier::Meta;
                modifier = modifier & ~togglable;
                if (ctrl) modifier |= KeyModifier::Ctrl;
                if (shift) modifier |= KeyModifier::Shift;
                if (alt) modifier |= KeyModifier::Alt;
                if (win) modifier |= KeyModifier::Meta;
            }
            return changed;
        }

        bool renderKeyEntry(std::vector<std::string>& keys, size_t index, InputCaptureState& capture, bool& removed)
        {
            auto& loc = Localization::get();
            bool changed = false;
            removed = false;

            ImGuiIdScope idxScope{ static_cast<int>(index) };

            Key key = Key::fromString(keys[index]);
            std::vector<std::string> chips = modifierChips(key.modifier);
            chips.push_back(keyBaseDisplayName(key));
            renderChips(chips);

            ImGui::SameLine();
            if (ImGui::SmallButton(loc.text("buttons.remove").data()))
            {
                keys.erase(keys.begin() + static_cast<std::ptrdiff_t>(index));
                if (std::cmp_equal(capture.keyIndex, index)) { capture.keyIndex = -1; }
                else if (std::cmp_greater(capture.keyIndex, index)) { capture.keyIndex--; }
                removed = true;
                return true;
            }

            ImGui::SameLine();
            if (std::cmp_equal(capture.keyIndex, index))
            {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", loc.text("status.recordingInputPrompt").data());
                ImGui::SameLine();
                if (ImGui::SmallButton(loc.text("buttons.cancel").data()))
                {
                    capture.keyIndex = -1;
                }
            }
            else if (ImGui::SmallButton(loc.text("buttons.record").data()))
            {
                capture.keyIndex = static_cast<int>(index);
            }

            if (modifierCheckboxes(key.modifier))
            {
                keys[index] = key.toString();
                changed = true;
            }

            return changed;
        }

        bool renderButtonEntry(std::vector<std::string>& buttons, size_t index, InputCaptureState& capture, bool& removed)
        {
            auto& loc = Localization::get();
            bool changed = false;
            removed = false;

            ImGuiIdScope idxScope{ static_cast<int>(index) };

            Mouse mouse = Mouse::fromString(buttons[index]);
            std::vector<std::string> chips = modifierChips(mouse.modifier);
            chips.push_back(buttonBaseDisplayName(mouse));
            renderChips(chips);

            ImGui::SameLine();
            if (ImGui::SmallButton(loc.text("buttons.remove").data()))
            {
                buttons.erase(buttons.begin() + static_cast<std::ptrdiff_t>(index));
                if (std::cmp_equal(capture.buttonIndex, index)) { capture.buttonIndex = -1; }
                else if (std::cmp_greater(capture.buttonIndex, index)) { capture.buttonIndex--; }
                removed = true;
                return true;
            }

            ImGui::SameLine();
            if (std::cmp_equal(capture.buttonIndex, index))
            {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", loc.text("status.recordingInputPrompt").data());
                ImGui::SameLine();
                if (ImGui::SmallButton(loc.text("buttons.cancel").data()))
                {
                    capture.buttonIndex = -1;
                }
            }
            else if (ImGui::SmallButton(loc.text("buttons.record").data()))
            {
                capture.buttonIndex = static_cast<int>(index);
            }

            if (modifierCheckboxes(mouse.modifier))
            {
                buttons[index] = mouse.toString();
                changed = true;
            }

            return changed;
        }
    }

    bool InputComboListEditor(
        const char* label,
        std::vector<std::string>& keys,
        std::vector<std::string>& buttons,
        InputCaptureState& capture)
    {
        auto& loc = Localization::get();
        bool changed = false;

        ImGui::PushID(label);
        ImGui::TextUnformatted(label);
        ImGui::TextWrapped("%s", loc.text("labels.inputsDescription").data());
        ImGui::TextDisabled("%s", loc.text("labels.inputsModifierHint").data());

        ImGui::Indent();
        for (size_t i = 0; i < keys.size();)
        {
            bool removed = false;
            if (renderKeyEntry(keys, i, capture, removed))
            {
                changed = true;
            }
            if (!removed)
            {
                ++i;
            }
        }
        for (size_t i = 0; i < buttons.size();)
        {
            bool removed = false;
            if (renderButtonEntry(buttons, i, capture, removed))
            {
                changed = true;
            }
            if (!removed)
            {
                ++i;
            }
        }
        ImGui::Unindent();

        if (keys.empty() && buttons.empty())
        {
            ImGui::TextDisabled("%s", loc.text("labels.inputsNoneConfigured").data());
        }

        ImGui::Separator();
        ImGui::TextUnformatted(loc.text("labels.inputsModifiersForNew").data());

        const ImGuiID ctrlId = ImGui::GetID("add_ctrl");
        const ImGuiID shiftId = ImGui::GetID("add_shift");
        const ImGuiID altId = ImGui::GetID("add_alt");
        const ImGuiID winId = ImGui::GetID("add_win");
        ImGuiStorage* storage = ImGui::GetStateStorage();

        bool addCtrl = storage->GetBool(ctrlId, false);
        bool addShift = storage->GetBool(shiftId, false);
        bool addAlt = storage->GetBool(altId, false);
        bool addWin = storage->GetBool(winId, false);

        const std::string addCtrlLabel = std::string(loc.text("labels.modifierCtrl")) + "##add";
        const std::string addShiftLabel = std::string(loc.text("labels.modifierShift")) + "##add";
        const std::string addAltLabel = std::string(loc.text("labels.modifierAlt")) + "##add";
        const std::string addWinLabel = std::string(loc.text("labels.modifierWin")) + "##add";

        if (ImGui::Checkbox(addCtrlLabel.c_str(), &addCtrl))
        {
            storage->SetBool(ctrlId, addCtrl);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox(addShiftLabel.c_str(), &addShift))
        {
            storage->SetBool(shiftId, addShift);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox(addAltLabel.c_str(), &addAlt))
        {
            storage->SetBool(altId, addAlt);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox(addWinLabel.c_str(), &addWin))
        {
            storage->SetBool(winId, addWin);
        }

        auto newModifier = KeyModifier::None;
        if (addCtrl)
        {
            newModifier |= KeyModifier::Ctrl;
        }
        if (addShift)
        {
            newModifier |= KeyModifier::Shift;
        }
        if (addAlt)
        {
            newModifier |= KeyModifier::Alt;
        }
        if (addWin)
        {
            newModifier |= KeyModifier::Meta;
        }

        const ImGuiID keyBufferId = ImGui::GetID("add_key_buffer");
        std::string& newKeyText = newKeyBuffers()[keyBufferId];
        ImGui::SetNextItemWidth(120.0f);
        ImGui::InputTextWithHint("##newKey", loc.text("labels.inputsKeyHint").data(), &newKeyText);
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.addKey").data()) && !newKeyText.empty())
        {
            Key key = Key::fromString(newKeyText);
            key.modifier |= newModifier;
            if (!key.character.empty())
            {
                keys.push_back(key.toString());
                newKeyText.clear();
                changed = true;
            }
        }

        const ImGuiID buttonSelectId = ImGui::GetID("add_button_select");
        int selectedButton = storage->GetInt(buttonSelectId, 0);
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::BeginCombo("##newButton", loc.text(knownMouseButtons[selectedButton].second).data()))
        {
            for (int b = 0; b < static_cast<int>(knownMouseButtons.size()); ++b)
            {
                const bool isSelected = (b == selectedButton);
                if (ImGui::Selectable(loc.text(knownMouseButtons[b].second).data(), isSelected))
                {
                    selectedButton = b;
                    storage->SetInt(buttonSelectId, selectedButton);
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.addMouseButton").data()))
        {
            Mouse mouse = Mouse::fromString(knownMouseButtons[selectedButton].first);
            mouse.modifier |= newModifier;
            buttons.push_back(mouse.toString());
            changed = true;
        }

        ImGui::PopID();
        return changed;
    }
}
