/**
 * @file validationReportWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "validationReportWindow.h"
#include "../widgets/basicWidgets.h"
#include "../core/windowManager.h"
#include "../core/windowIds.h"
#include "../core/localization.h"
#include "configEditorWindow.h"
#include "autoinput/logger.h"
#include <imgui.h>
#include <format>
#include <algorithm>
#include <map>

namespace autoinput::ui::windows
{
    ValidationReportWindow::ValidationReportWindow(WindowManager& windowManager, const IEnvironment& environment)
        : UiWindow("Validation Report", "windows.validationReport")
        , m_windowManager(windowManager)
        , m_environment(environment)
        , m_configService(environment)
    {
        m_settings.load();
    }

    void ValidationReportWindow::renderContent()
    {
        auto& loc = Localization::get();
        // Toolbar
        if (ImGui::Button(loc.text("buttons.validateCurrent").data()))
        {
            runCurrentConfigValidation();
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.validateAllConfigs").data()))
        {
            runAllConfigsValidation();
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.validateSettings").data()))
        {
            runSettingsValidation();
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.clearResults").data()))
        {
            clearResults();
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.copyReport").data()))
        {
            copyReport();
        }

        if (!m_statusMessage.empty())
        {
            widgets::StatusText(m_statusMessage);
        }

        ImGui::Separator();

        if (m_issues.empty())
        {
            ImGui::Text("%s", loc.text("labels.noIssues").data());
        }
        else
        {
            // Group issues by sourceName
            std::map<std::string, std::vector<size_t>> groupedIssues;
            for (size_t i = 0; i < m_issues.size(); ++i)
            {
                groupedIssues[m_issues[i].sourceName].push_back(i);
            }

            if (ImGui::BeginChild("IssuesList"))
            {
                for (const auto& [sourceName, indices] : groupedIssues)
                {
                    bool isSettings = m_issues[indices[0]].type == "Settings";
                    
                    ImGui::PushID(sourceName.c_str());
                    bool open = ImGui::CollapsingHeader(sourceName.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
                    
                    if (!isSettings && !m_issues[indices[0]].sourcePath.empty())
                    {
                        ImGui::SameLine(ImGui::GetWindowWidth() - 120);
                        if (ImGui::SmallButton(loc.text("buttons.openInEditor").data()))
                        {
                            openInEditor(m_issues[indices[0]]);
                        }
                    }

                    if (open)
                    {
                        for (size_t index : indices)
                        {
                            const auto& issue = m_issues[index];
                            const auto& err = issue.error;

                            ImVec4 color;
                            const char* severityPrefix = "";
                            switch (err.severity)
                            {
                            case ValidationSeverity::Info:
                                color = ImVec4(0.5f, 0.5f, 0.9f, 1.0f);
                                severityPrefix = "[INFO] ";
                                break;
                            case ValidationSeverity::Warning:
                                color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                                severityPrefix = "[WARN] ";
                                break;
                            case ValidationSeverity::Error:
                                color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
                                severityPrefix = "[ERR ] ";
                                break;
                            }

                            ImGui::PushStyleColor(ImGuiCol_Text, color);
                            std::string label = severityPrefix;
                            if (!err.section.empty())
                            {
                                if (!err.field.empty())
                                    label += std::format("{}.{}: ", err.section, err.field);
                                else
                                    label += std::format("{}: ", err.section);
                            }
                            label += err.message;

                            ImGui::BulletText("%s", label.c_str());
                            ImGui::PopStyleColor();

                            if (!err.suggestedFix.empty())
                            {
                                ImGui::Indent();
                                ImGui::TextDisabled("%s: %s", loc.text("labels.suggestedFix").data(), err.suggestedFix.c_str());
                                ImGui::Unindent();
                            }
                        }
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndChild();
        }
    }

    void ValidationReportWindow::runCurrentConfigValidation()
    {
        auto& loc = Localization::get();
        std::string current = m_configService.getCurrentConfig();
        if (current.empty())
        {
            m_statusMessage = loc.text("status.noConfigSelected");
            return;
        }

        // Clear previous results for this config
        std::erase_if(m_issues, [&](const auto& issue) {
            return issue.sourceName == current;
        });

        auto result = m_configService.validateConfig(current);
        for (const auto& err : result.errors)
        {
            m_issues.push_back(ValidationIssue{ current, result.configPath, "Config", err });
        }

        m_statusMessage = loc.format("status.validatedCurrentConfig", current, result.errors.size());
    }

    void ValidationReportWindow::runAllConfigsValidation()
    {
        auto& loc = Localization::get();
        auto configs = m_configService.listAvailableConfigs();
        
        // Filter out existing config issues
        std::erase_if(m_issues, [](const auto& issue) {
            return issue.type == "Config";
        });

        size_t totalIssues = 0;
        for (const auto& info : configs)
        {
            std::string name = info.fileStem();
            auto result = m_configService.validateConfig(info.filepath.string());
            for (const auto& err : result.errors)
            {
                m_issues.push_back(ValidationIssue{ name, info.filepath.string(), "Config", err });
                totalIssues++;
            }
        }

        m_statusMessage = loc.format("status.validatedAllConfigs", configs.size(), totalIssues);
    }

    void ValidationReportWindow::runSettingsValidation()
    {
        auto& loc = Localization::get();
        m_settings.load();
        
        // Clear previous settings issues
        std::erase_if(m_issues, [](const auto& issue) {
            return issue.type == "Settings";
        });

        auto errors = autoinput::validateSettings(m_settings.getDefaults());
        for (const auto& err : errors)
        {
            m_issues.push_back(ValidationIssue{ std::string(loc.text("labels.globalSettings")), "", "Settings", err });
        }

        m_statusMessage = loc.format("status.validatedSettings", errors.size());
    }

    void ValidationReportWindow::clearResults()
    {
        m_issues.clear();
        m_statusMessage = Localization::get().text("status.resultsCleared");
    }

    void ValidationReportWindow::copyReport()
    {
        auto& loc = Localization::get();
        std::string report = std::string(loc.text("labels.reportTitle")) + "\n===========================\n\n";
        if (m_issues.empty())
        {
            report += std::string(loc.text("labels.noIssuesFound")) + "\n";
        }
        else
        {
            for (const auto& issue : m_issues)
            {
                const char* severity = "INFO";
                if (issue.error.severity == ValidationSeverity::Warning) severity = "WARN";
                else if (issue.error.severity == ValidationSeverity::Error) severity = "ERR";

                report += std::format("[{}] Source: {} ({})\n", severity, issue.sourceName, issue.type);
                if (!issue.error.section.empty())
                {
                    report += std::format("  {}: {}", loc.text("labels.location").data(), issue.error.section);
                    if (!issue.error.field.empty()) report += std::format(".{}", issue.error.field);
                    report += "\n";
                }
                report += std::format("  {}: {}\n", loc.text("labels.message").data(), issue.error.message);
                if (!issue.error.suggestedFix.empty())
                {
                    report += std::format("  {}: {}\n", loc.text("labels.fix").data(), issue.error.suggestedFix);
                }
                report += "\n";
            }
        }

        ImGui::SetClipboardText(report.c_str());
        m_statusMessage = loc.text("status.reportCopied");
    }

    void ValidationReportWindow::openInEditor(const ValidationIssue& issue)
    {
        if (issue.sourcePath.empty()) return;

        auto editor = m_windowManager.findAs<ConfigEditorWindow>(WindowIds::ConfigEditor);
        if (editor)
        {
            editor->loadConfig(issue.sourcePath);
            editor->open();
        }
    }
}
