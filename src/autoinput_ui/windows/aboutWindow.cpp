/**
 * @file aboutWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */

#include "aboutWindow.h"

namespace autoinput::ui
{
    void AboutWindow::renderContent()
    {
        ImGui::Text("AutoInput");
        ImGui::Text("Version: %s", AUTOINPUT_VERSION);
        ImGui::Text("License: MIT");

        ImGui::Separator();

        if (ImGui::Button("Close"))
        {
            requestClose();
        }
    }
}
