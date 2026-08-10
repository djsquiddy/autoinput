/**
 * @file aboutWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */

#include "aboutWindow.h"
#include "../core/localization.h"
#include "autoinput/support/logger.h"
#include "autoinput/config/config.h"
#include "autoinput/config/defaults.h"
#include <imgui.h>
#include <format>
#include <string>

namespace autoinput::ui
{
    namespace
    {
        std::string getCompilerInfo()
        {
            auto& loc = Localization::get();
#if defined(_MSC_VER)
            return std::format("MSVC {}.{}.{}", _MSC_VER / 100, _MSC_VER % 100, _MSC_FULL_VER % 100000);
#elif defined(__clang__)
            return std::format("Clang {}.{}.{}", __clang_major__, __clang_minor__, __clang_patchlevel__);
#elif defined(__GNUC__)
            return std::format("GCC {}.{}.{}", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#else
            return std::string(loc.text("labels.unknownCompiler"));
#endif
        }
 
        std::string getBuildConfig()
        {
            auto& loc = Localization::get();
#if defined(DEBUG) || defined(_DEBUG)
            return std::string(loc.text("labels.debug"));
#else
            return std::string(loc.text("labels.release"));
#endif
        }
    }

    void AboutWindow::renderContent()
    {
        auto& loc = Localization::get();
        ImGui::Text("%s", loc.text("app.name").data());
        ImGui::Text("%s: %s", loc.text("labels.version").data(), AUTOINPUT_VERSION);
        ImGui::Text("%s: MIT", loc.text("labels.license").data());
        ImGui::Spacing();
 
        if (ImGui::CollapsingHeader(loc.text("labels.diagnostics").data(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            const auto platform = m_environment.platformName();
            const auto buildConfig = getBuildConfig();
            const auto compiler = getCompilerInfo();
            const auto configDir = autoinput::getConfigsPath(m_environment).string();
            const auto userConfigDir = autoinput::getUserConfigsPath(m_environment).string();
            const auto settingsPath = (autoinput::getUserConfigsPath(m_environment) / autoinput::defaults::SettingFileName).string();
            const auto logPath = autoinput::Logger::getFileName();
            const auto backend = m_runtimeClient.getBackendName();
            const auto rawStatus = services::statusToString(m_runtimeClient.getStatus());
            std::string status = rawStatus;
            if (rawStatus == "Running") status = loc.text("status.running");
            else if (rawStatus == "Stopped") status = loc.text("status.stopped");
            else if (rawStatus == "Starting") status = loc.text("status.starting");
            else if (rawStatus == "Paused") status = loc.text("status.paused");
            else if (rawStatus == "Error") status = loc.text("status.error");
            else if (rawStatus == "Unknown") status = loc.text("status.unknown");
 
            if (ImGui::BeginTable("DiagnosticsTable", 2, ImGuiTableFlags_BordersInnerV))
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("%s:", loc.text("labels.platform").data());
                ImGui::TableSetColumnIndex(1); ImGui::Text("%s", platform.c_str());
 
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("%s:", loc.text("labels.buildConfig").data());
                ImGui::TableSetColumnIndex(1); ImGui::Text("%s", buildConfig.c_str());
 
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("%s:", loc.text("labels.compiler").data());
                ImGui::TableSetColumnIndex(1); ImGui::Text("%s", compiler.c_str());
 
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("%s:", loc.text("labels.activeBackend").data());
                ImGui::TableSetColumnIndex(1); ImGui::Text("%s", backend.empty() ? loc.text("labels.none").data() : backend.c_str());
 
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("%s:", loc.text("labels.runtimeStatus").data());
                ImGui::TableSetColumnIndex(1); ImGui::Text("%s", status.c_str());
 
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("%s:", loc.text("labels.globalConfigDir").data());
                ImGui::TableSetColumnIndex(1); ImGui::TextWrapped("%s", configDir.c_str());
 
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("%s:", loc.text("labels.userConfigDir").data());
                ImGui::TableSetColumnIndex(1); ImGui::TextWrapped("%s", userConfigDir.c_str());
 
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("%s:", loc.text("labels.settingsFile").data());
                ImGui::TableSetColumnIndex(1); ImGui::TextWrapped("%s", settingsPath.c_str());
 
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("%s:", loc.text("labels.logFile").data());
                ImGui::TableSetColumnIndex(1); ImGui::TextWrapped("%s", logPath.c_str());
 
                ImGui::EndTable();
            }
 
            ImGui::Spacing();
            if (ImGui::Button(loc.text("buttons.copyDiagnostics").data()))
            {
                std::string report = std::format(
                    "AutoInput Diagnostic Report\n"
                    "===========================\n"
                    "{}: {}\n"
                    "{}: {}\n"
                    "{}: {}\n"
                    "{}: {}\n"
                    "{}: {}\n"
                    "{}: {}\n"
                    "{}: {}\n"
                    "{}: {}\n"
                    "{}: {}\n"
                    "{}: {}\n",
                    loc.text("labels.version"), AUTOINPUT_VERSION,
                    loc.text("labels.platform"), platform,
                    loc.text("labels.buildConfig"), buildConfig,
                    loc.text("labels.compiler"), compiler,
                    loc.text("labels.activeBackend"), backend.empty() ? loc.text("labels.none").data() : backend,
                    loc.text("labels.runtimeStatus"), status,
                    loc.text("labels.globalConfigDir"), configDir,
                    loc.text("labels.userConfigDir"), userConfigDir,
                    loc.text("labels.settingsFile"), settingsPath,
                    loc.text("labels.logFile"), logPath
                );
                ImGui::SetClipboardText(report.c_str());
            }
 
            ImGui::SameLine();
            if (ImGui::Button(loc.text("buttons.openConfigFolder").data()))
            {
                m_environment.openPath(autoinput::getUserConfigsPath(m_environment));
            }
 
            ImGui::SameLine();
            if (ImGui::Button(loc.text("buttons.openLogFolder").data()))
            {
                std::filesystem::path logFile(logPath);
                if (logFile.has_parent_path())
                {
                    m_environment.openPath(logFile.parent_path());
                }
            }
        }
 
        ImGui::Separator();
 
        if (ImGui::Button(loc.text("buttons.close").data()))
        {
            requestClose();
        }
    }
}
