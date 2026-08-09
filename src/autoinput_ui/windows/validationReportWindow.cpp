/**
 * @file validationReportWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "validationReportWindow.h"
#include "../widgets/basicWidgets.h"
#include "../core/windowManager.h"
#include "configEditorWindow.h"
#include "autoinput/logger.h"
#include <imgui.h>
#include <format>
#include <algorithm>
#include <map>

namespace autoinput::ui::windows
{
    ValidationReportWindow::ValidationReportWindow(WindowManager& windowManager, const IEnvironment& environment)
        : UiWindow("Validation Report")
        , m_windowManager(windowManager)
        , m_environment(environment)
        , m_configService(environment)
    {
        m_settings.load();
    }

    void ValidationReportWindow::renderContent()
    {
        // Toolbar
        if (ImGui::Button("Validate Current Config"))
        {
            runCurrentConfigValidation();
        }
        ImGui::SameLine();
        if (ImGui::Button("Validate All Configs"))
        {
            runAllConfigsValidation();
        }
        ImGui::SameLine();
        if (ImGui::Button("Validate Settings"))
        {
            runSettingsValidation();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear Results"))
        {
            clearResults();
        }
        ImGui::SameLine();
        if (ImGui::Button("Copy Report"))
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
            ImGui::Text("No validation issues found or validation not run yet.");
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
                        if (ImGui::SmallButton("Open in Editor"))
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
                                ImGui::TextDisabled("Suggested Fix: %s", err.suggestedFix.c_str());
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
        std::string current = m_configService.getCurrentConfig();
        if (current.empty())
        {
            m_statusMessage = "No config currently selected.";
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

        m_statusMessage = std::format("Validated current config: {}. {} issues found.", current, result.errors.size());
    }

    void ValidationReportWindow::runAllConfigsValidation()
    {
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

        m_statusMessage = std::format("Validated {} configs. {} issues found.", configs.size(), totalIssues);
    }

    void ValidationReportWindow::runSettingsValidation()
    {
        m_settings.load();
        
        // Clear previous settings issues
        std::erase_if(m_issues, [](const auto& issue) {
            return issue.type == "Settings";
        });

        auto errors = autoinput::validateSettings(m_settings.getDefaults());
        for (const auto& err : errors)
        {
            m_issues.push_back(ValidationIssue{ "Global Settings", "", "Settings", err });
        }

        m_statusMessage = std::format("Validated global settings. {} issues found.", errors.size());
    }

    void ValidationReportWindow::clearResults()
    {
        m_issues.clear();
        m_statusMessage = "Results cleared.";
    }

    void ValidationReportWindow::copyReport()
    {
        std::string report = "AutoInput Validation Report\n===========================\n\n";
        if (m_issues.empty())
        {
            report += "No issues found.\n";
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
                    report += std::format("  Location: {}", issue.error.section);
                    if (!issue.error.field.empty()) report += std::format(".{}", issue.error.field);
                    report += "\n";
                }
                report += std::format("  Message: {}\n", issue.error.message);
                if (!issue.error.suggestedFix.empty())
                {
                    report += std::format("  Fix: {}\n", issue.error.suggestedFix);
                }
                report += "\n";
            }
        }

        ImGui::SetClipboardText(report.c_str());
        m_statusMessage = "Report copied to clipboard.";
    }

    void ValidationReportWindow::openInEditor(const ValidationIssue& issue)
    {
        if (issue.sourcePath.empty()) return;

        auto editor = m_windowManager.findAs<ConfigEditorWindow>("config-editor");
        if (editor)
        {
            editor->loadConfig(issue.sourcePath);
            editor->open();
        }
    }
}
