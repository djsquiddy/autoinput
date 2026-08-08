/**
 * @file globalSettingsEditor.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "globalSettingsEditor.h"
#include "../widgets/formWidgets.h"
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

        if (widgets::StringInput("End Hotkey", settings.endKey))
        {
            changed = true;
        }

        if (widgets::StringInput("Application", settings.application))
        {
            changed = true;
        }

        if (ImGui::Checkbox("Append Blacklist", &settings.appendBlacklist))
        {
            changed = true;
        }

        if (widgets::StringListEditor("Blacklist (Application names)", settings.blacklist, "Add Application"))
        {
            changed = true;
        }

        if (widgets::StringCombo("Notification Mode", settings.statusNotificationMode, statusNotificationModes))
        {
            changed = true;
        }

        if (widgets::StringCombo("Log Level", settings.logLevel, logLevels))
        {
            changed = true;
        }

        return changed;
    }
}
