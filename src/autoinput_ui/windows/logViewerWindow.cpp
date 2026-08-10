/**
 * @file logViewerWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "logViewerWindow.h"
#include "../core/localization.h"
#include <imgui.h>
#include <algorithm>
#include <format>

namespace autoinput::ui
{
    LogViewerWindow::LogViewerWindow(const IEnvironment& environment)
        : UiWindow("Logs", "windows.logs"), m_environment(environment)
    {
    }

    void LogViewerWindow::onOpen()
    {
        refreshLogs();
        m_lastRefreshTime = std::chrono::steady_clock::now();
    }

    void LogViewerWindow::update()
    {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastRefreshTime).count() > 500)
        {
            refreshLogs();
            m_lastRefreshTime = now;
        }
    }

    void LogViewerWindow::refreshLogs()
    {
        m_logs = Logger::getRecentLogs();
        applyFilters();
        if (m_autoScroll)
        {
            m_scrollToBottom = true;
        }
    }

    void LogViewerWindow::clearLogs()
    {
        Logger::clearRecentLogs();
        m_logs.clear();
        m_filteredLogs.clear();
    }

    void LogViewerWindow::copyToClipboard()
    {
        std::string allText;
        for (const auto& entry : m_filteredLogs)
        {
            allText += std::format("[{}] {} | {}:{}:{} | {}\n",
                entry.level,
                entry.timestamp,
                entry.file,
                entry.function,
                entry.line,
                entry.message
            );
        }
        ImGui::SetClipboardText(allText.c_str());
    }

    void LogViewerWindow::openLogFile()
    {
        const std::string& fileName = Logger::getFileName();
        if (!fileName.empty())
        {
            m_environment.openPath(std::filesystem::absolute(fileName));
        }
    }

    void LogViewerWindow::openLogFolder()
    {
        const std::string& fileName = Logger::getFileName();
        if (!fileName.empty())
        {
            m_environment.openPath(std::filesystem::absolute(fileName).parent_path());
        }
    }

    void LogViewerWindow::applyFilters()
    {
        m_filteredLogs.clear();
        std::string searchStr = m_searchBuffer;
        std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

        for (const auto& entry : m_logs)
        {
            // Level filter
            bool levelMatch = false;
            switch (m_selectedLogLevel)
            {
            case 0: levelMatch = true; break; // All
            case 1: levelMatch = (entry.level >= LogLevel::Trace); break;
            case 2: levelMatch = (entry.level >= LogLevel::Debug); break;
            case 3: levelMatch = (entry.level >= LogLevel::Info); break;
            case 4: levelMatch = (entry.level >= LogLevel::Warning); break;
            case 5: levelMatch = (entry.level >= LogLevel::Error); break;
            default: levelMatch = true; break;
            }

            if (!levelMatch) continue;

            // Search filter
            if (!searchStr.empty())
            {
                std::string msgLower = entry.message;
                std::transform(msgLower.begin(), msgLower.end(), msgLower.begin(), ::tolower);
                if (msgLower.find(searchStr) == std::string::npos)
                {
                    continue;
                }
            }

            m_filteredLogs.push_back(entry);
        }
    }

    void LogViewerWindow::renderContent()
    {
        auto& loc = Localization::get();
        // Toolbar
        if (ImGui::Button(loc.text("buttons.refresh").data()))
        {
            refreshLogs();
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.clearView").data()))
        {
            clearLogs();
        }
        ImGui::SameLine();
        if (ImGui::Checkbox(loc.text("buttons.autoScroll").data(), &m_autoScroll))
        {
            if (m_autoScroll) m_scrollToBottom = true;
        }
        
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        // We still need the raw strings for the combo for now, or localize them individually.
        // For simplicity I'll keep the array but prefix with localized "All".
        const std::string allText = std::string(loc.text("labels.all"));
        const char* levels[] = { allText.c_str(), "Trace+", "Debug+", "Info+", "Warning+", "Error+" };
        if (ImGui::Combo(loc.text("labels.filter").data(), &m_selectedLogLevel, levels, IM_ARRAYSIZE(levels)))
        {
            applyFilters();
        }
 
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        if (ImGui::InputText(loc.text("labels.search").data(), m_searchBuffer, sizeof(m_searchBuffer)))
        {
            applyFilters();
        }

        ImGui::Separator();

        // Log Path and Open buttons
        const std::string& logFile = Logger::getFileName();
        if (!logFile.empty())
        {
            ImGui::Text("%s: %s", loc.text("labels.logFile").data(), logFile.c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton(loc.text("buttons.openFile").data()))
            {
                openLogFile();
            }
            ImGui::SameLine();
            if (ImGui::SmallButton(loc.text("buttons.openFolder").data()))
            {
                openLogFolder();
            }
            ImGui::SameLine();
        }
        else
        {
            ImGui::Text("%s: %s", loc.text("labels.logFile").data(), loc.text("labels.notConfigured").data());
            ImGui::SameLine();
        }
        
        if (ImGui::SmallButton(loc.text("buttons.copyAll").data()))
        {
            copyToClipboard();
        }

        ImGui::Separator();

        // Scrollable region
        const float footerHeightToReserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerHeightToReserve), false, ImGuiWindowFlags_HorizontalScrollbar);
        
        if (ImGui::BeginTable("LogTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn(loc.text("labels.timeLevel").data(), ImGuiTableColumnFlags_WidthFixed, 180.0f);
            ImGui::TableSetupColumn(loc.text("labels.message").data(), ImGuiTableColumnFlags_WidthStretch);
            // ImGui::TableHeadersRow();

            for (const auto& entry : m_filteredLogs)
            {
                ImVec4 color;
                bool hasColor = true;
                switch (entry.level)
                {
                case LogLevel::Trace:   color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f); break; // Gray
                case LogLevel::Debug:   color = ImVec4(0.3f, 0.6f, 1.0f, 1.0f); break; // Blue
                case LogLevel::Info:    color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f); break; // Green
                case LogLevel::Warning: color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); break; // Yellow
                case LogLevel::Error:   color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); break; // Red
                case LogLevel::Fatal:   color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); break; // Bright Red
                default: hasColor = false; break;
                }

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (hasColor) ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::Text("[%s] %s", entry.timestamp.c_str(), logLevelToString(entry.level).c_str());
                
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(entry.message.c_str());
                if (hasColor) ImGui::PopStyleColor();

                // Tooltip for location info
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("File: %s", entry.file.c_str());
                    ImGui::Text("Func: %s", entry.function.c_str());
                    ImGui::Text("Line: %d", entry.line);
                    ImGui::EndTooltip();
                }
            }

            if (m_scrollToBottom || (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
            {
                ImGui::SetScrollHereY(1.0f);
                m_scrollToBottom = false;
            }

            ImGui::EndTable();
        }
        ImGui::EndChild();
    }
}
