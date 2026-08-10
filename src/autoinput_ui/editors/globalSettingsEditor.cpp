/**
 * @file globalSettingsEditor.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "globalSettingsEditor.h"
#include "../widgets/formWidgets.h"
#include "../core/localization.h"
#include <imgui.h>
#include <array>

namespace autoinput::ui::editors
{
    namespace
    {
        constexpr std::array<std::string_view, 4> statusNotificationModes = { "off", "console", "desktop", "both" };
        constexpr std::array<std::string_view, 4> logLevels = { "debug", "info", "warning", "error" };
    }

    bool renderGlobalSettingsEditor(GlobalSettings& settings)
    {
        bool changed = false;
        auto& loc = Localization::get();
 
        if (widgets::StringInput(loc.text("labels.endHotkey").data(), settings.endKey))
        {
            changed = true;
        }
 
        if (widgets::StringInput(loc.text("labels.application").data(), settings.application))
        {
            changed = true;
        }
 
        if (ImGui::Checkbox(loc.text("labels.appendBlacklist").data(), &settings.appendBlacklist))
        {
            changed = true;
        }
 
        if (widgets::StringListEditor(loc.text("labels.blacklistDescription").data(), settings.blacklist, loc.text("buttons.addApplication").data()))
        {
            changed = true;
        }
 
        if (widgets::StringCombo(loc.text("labels.notificationMode").data(), settings.statusNotificationMode, statusNotificationModes))
        {
            changed = true;
        }
 
        if (widgets::StringCombo(loc.text("labels.logLevel").data(), settings.logLevel, logLevels))
        {
            changed = true;
        }
 
        // Language selector
        auto availableLanguages = Localization::getAvailableLanguages();
        if (widgets::StringCombo(loc.text("labels.uiLanguage").data(), settings.uiLanguage, availableLanguages))
        {
            changed = true;
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "%s", loc.text("labels.restartRequired").data());
        }
 
        return changed;
    }
}
