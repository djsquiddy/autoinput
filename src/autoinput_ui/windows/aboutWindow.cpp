/**
 * @file aboutWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */

#include "aboutWindow.h"
#include "autoinput/logger.h"
#include "autoinput/config.h"
#include "autoinput/defaults.h"
#include <imgui.h>
#include <format>
#include <string>

namespace autoinput::ui
{
    namespace
    {
        std::string getCompilerInfo()
        {
#if defined(_MSC_VER)
            return std::format("MSVC {}.{}.{}", _MSC_VER / 100, _MSC_VER % 100, _MSC_FULL_VER % 100000);
#elif defined(__clang__)
            return std::format("Clang {}.{}.{}", __clang_major__, __clang_minor__, __clang_patchlevel__);
#elif defined(__GNUC__)
            return std::format("GCC {}.{}.{}", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#else
            return "Unknown Compiler";
#endif
        }

        std::string getBuildConfig()
        {
#if defined(DEBUG) || defined(_DEBUG)
            return "Debug";
#else
            return "Release";
#endif
        }
    }

    void AboutWindow::renderContent()
    {
        ImGui::Text("AutoInput");
        ImGui::Text("Version: %s", AUTOINPUT_VERSION);
        ImGui::Text("License: MIT");
        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Diagnostics", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const auto platform = m_environment.platformName();
            const auto buildConfig = getBuildConfig();
            const auto compiler = getCompilerInfo();
            const auto configDir = autoinput::getConfigsPath(m_environment).string();
            const auto userConfigDir = autoinput::getUserConfigsPath(m_environment).string();
            const auto settingsPath = (autoinput::getUserConfigsPath(m_environment) / autoinput::defaults::SettingFileName).string();
            const auto logPath = autoinput::Logger::getFileName();
            const auto backend = m_runtimeClient.getBackendName();
            const auto status = services::statusToString(m_runtimeClient.getStatus());

            if (ImGui::BeginTable("DiagnosticsTable", 2, ImGuiTableFlags_BordersInnerV))
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("Platform:");
                ImGui::TableSetColumnIndex(1); ImGui::Text("%s", platform.c_str());

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("Build Config:");
                ImGui::TableSetColumnIndex(1); ImGui::Text("%s", buildConfig.c_str());

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("Compiler:");
                ImGui::TableSetColumnIndex(1); ImGui::Text("%s", compiler.c_str());

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("Active Backend:");
                ImGui::TableSetColumnIndex(1); ImGui::Text("%s", backend.empty() ? "None" : backend.c_str());

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("Runtime Status:");
                ImGui::TableSetColumnIndex(1); ImGui::Text("%s", status);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("Global Config Dir:");
                ImGui::TableSetColumnIndex(1); ImGui::TextWrapped("%s", configDir.c_str());

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("User Config Dir:");
                ImGui::TableSetColumnIndex(1); ImGui::TextWrapped("%s", userConfigDir.c_str());

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("Settings File:");
                ImGui::TableSetColumnIndex(1); ImGui::TextWrapped("%s", settingsPath.c_str());

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("Log File:");
                ImGui::TableSetColumnIndex(1); ImGui::TextWrapped("%s", logPath.c_str());

                ImGui::EndTable();
            }

            ImGui::Spacing();
            if (ImGui::Button("Copy Diagnostics"))
            {
                std::string report = std::format(
                    "AutoInput Diagnostic Report\n"
                    "===========================\n"
                    "Version: {}\n"
                    "Platform: {}\n"
                    "Build Config: {}\n"
                    "Compiler: {}\n"
                    "Backend: {}\n"
                    "Status: {}\n"
                    "Global Config Dir: {}\n"
                    "User Config Dir: {}\n"
                    "Settings File: {}\n"
                    "Log File: {}\n",
                    AUTOINPUT_VERSION, platform, buildConfig, compiler, 
                    backend.empty() ? "None" : backend, status,
                    configDir, userConfigDir, settingsPath, logPath
                );
                ImGui::SetClipboardText(report.c_str());
            }

            ImGui::SameLine();
            if (ImGui::Button("Open Config Folder"))
            {
                m_environment.openPath(autoinput::getUserConfigsPath(m_environment));
            }

            ImGui::SameLine();
            if (ImGui::Button("Open Log Folder"))
            {
                std::filesystem::path logFile(logPath);
                if (logFile.has_parent_path())
                {
                    m_environment.openPath(logFile.parent_path());
                }
            }
        }

        ImGui::Separator();

        if (ImGui::Button("Close"))
        {
            requestClose();
        }
    }
}
