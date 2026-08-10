/**
 * @file backupRestoreWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "backupRestoreWindow.h"
#include "../widgets/basicWidgets.h"
#include "../core/localization.h"
#include "autoinput/logger.h"
#include "autoinput/config.h"
#include "autoinput/defaults.h"
#include "autoinput/environment.h"
#include "autoinput/configValidator.h"
#include <imgui.h>
#include <imgui_stdlib.h>
#include <filesystem>
#include <format>
#include <algorithm>
#include <chrono>

namespace autoinput::ui
{
    namespace fs = std::filesystem;

    BackupRestoreWindow::BackupRestoreWindow()
        : UiWindow("Backup / Restore", "windows.backupRestore")
    {
        refreshBackups();
    }

    void BackupRestoreWindow::onOpen()
    {
        refreshBackups();
        m_availableConfigs = autoinput::listAvailableConfigs();
        m_statusMessage.clear();
    }

    void BackupRestoreWindow::refreshBackups()
    {
        m_backups.clear();
        fs::path backupDir = getBackupDirPath();
        
        if (!fs::exists(backupDir))
        {
            return;
        }

        try
        {
            for (const auto& entry : fs::directory_iterator(backupDir))
            {
                if (entry.is_directory())
                {
                    BackupEntry backup;
                    backup.path = entry.path();
                    backup.name = entry.path().filename().string();
                    
                    // Parse type from name (e.g., backup_20260810_120000_all)
                    if (backup.name.find("_all") != std::string::npos)
                        backup.type = "All Configs";
                    else if (backup.name.find("_settings") != std::string::npos)
                        backup.type = "Settings";
                    else if (backup.name.find("_config_") != std::string::npos)
                        backup.type = "Single Config";
                    else
                        backup.type = "Unknown";

                    auto ftime = entry.last_write_time();
                    backup.created = std::chrono::clock_cast<std::chrono::system_clock>(ftime);
                    
                    // Calculate size of directory
                    uintmax_t totalSize = 0;
                    for (const auto& subEntry : fs::recursive_directory_iterator(entry.path()))
                    {
                        if (subEntry.is_regular_file())
                            totalSize += subEntry.file_size();
                    }
                    backup.size = totalSize;
                    
                    m_backups.push_back(backup);
                }
            }
            
            // Sort by creation time descending
            std::sort(m_backups.begin(), m_backups.end(), [](const BackupEntry& a, const BackupEntry& b) {
                return a.created > b.created;
            });
        }
        catch (const std::exception& e)
        {
            Logger::error("Failed to list backups: {}", e.what());
        }
    }

    fs::path BackupRestoreWindow::getBackupDirPath() const
    {
        return autoinput::getUserConfigsPath() / "backups";
    }

    void BackupRestoreWindow::renderContent()
    {
        auto& loc = Localization::get();
        renderControls();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        renderBackupTable();
 
        if (!m_statusMessage.empty())
        {
            ImVec4 color = m_statusIsError ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f) : ImVec4(0.4f, 1.0f, 0.4f, 1.0f);
            ImGui::TextColored(color, "%s", m_statusMessage.c_str());
        }
 
        // Modals
        if (m_showRestoreConfirm)
        {
            ImGui::OpenPopup(loc.text("modals.restoreBackupTitle").data());
            m_showRestoreConfirm = false;
        }
 
        if (ImGui::BeginPopupModal(loc.text("modals.restoreBackupTitle").data(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (m_pendingBackupIndex >= 0 && m_pendingBackupIndex < static_cast<int>(m_backups.size()))
            {
                const auto& entry = m_backups[m_pendingBackupIndex];
                ImGui::Text("%s", loc.format("modals.restoreBackupMessage", entry.name).c_str());
                ImGui::Spacing();
                
                if (ImGui::Button(loc.text("buttons.restore").data(), ImVec2(120, 0)))
                {
                    restoreBackup(entry);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button(loc.text("buttons.cancel").data(), ImVec2(120, 0)))
                {
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }
 
        if (m_showDeleteConfirm)
        {
            ImGui::OpenPopup(loc.text("modals.deleteBackupTitle").data());
            m_showDeleteConfirm = false;
        }
 
        if (ImGui::BeginPopupModal(loc.text("modals.deleteBackupTitle").data(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (m_pendingBackupIndex >= 0 && m_pendingBackupIndex < static_cast<int>(m_backups.size()))
            {
                const auto& entry = m_backups[m_pendingBackupIndex];
                ImGui::Text("%s", loc.format("modals.deleteBackupMessage", entry.name).c_str());
                ImGui::Spacing();
                
                if (ImGui::Button(loc.text("buttons.delete").data(), ImVec2(120, 0)))
                {
                    deleteBackup(entry);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button(loc.text("buttons.cancel").data(), ImVec2(120, 0)))
                {
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }
    }

    void BackupRestoreWindow::renderControls()
    {
        auto& loc = Localization::get();
        ImGui::Text("%s: %s", loc.text("labels.backupPath").data(), getBackupDirPath().string().c_str());
        ImGui::Spacing();
 
        if (ImGui::Button(loc.text("buttons.backupAll").data()))
        {
            backupAllConfigs();
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.backupSettings").data()))
        {
            backupSettings();
        }
 
        ImGui::Spacing();
 
        if (!m_availableConfigs.empty())
        {
            if (m_selectedConfigIndex >= static_cast<int>(m_availableConfigs.size()))
                m_selectedConfigIndex = 0;
 
            const char* preview = m_availableConfigs[m_selectedConfigIndex].c_str();
            ImGui::SetNextItemWidth(200);
            if (ImGui::BeginCombo("##config_select", preview))
            {
                for (int i = 0; i < static_cast<int>(m_availableConfigs.size()); ++i)
                {
                    const bool isSelected = (m_selectedConfigIndex == i);
                    if (ImGui::Selectable(m_availableConfigs[i].c_str(), isSelected))
                        m_selectedConfigIndex = i;
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            if (ImGui::Button(loc.text("buttons.backupSelected").data()))
            {
                backupSelectedConfig(m_availableConfigs[m_selectedConfigIndex]);
            }
        }
 
        ImGui::Spacing();
        if (ImGui::Button(loc.text("buttons.refresh").data()))
        {
            refreshBackups();
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.openFolder").data()))
        {
            SystemEnvironment::instance().openPath(getBackupDirPath());
        }
    }

    void BackupRestoreWindow::renderBackupTable()
    {
        auto& loc = Localization::get();
        if (m_backups.empty())
        {
            ImGui::Text("%s", loc.text("labels.noBackups").data());
            return;
        }
 
        if (ImGui::BeginTable("BackupsTable", 5, ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, ImVec2(0, 300)))
        {
            ImGui::TableSetupColumn(loc.text("labels.name").data());
            ImGui::TableSetupColumn(loc.text("labels.type").data());
            ImGui::TableSetupColumn(loc.text("labels.created").data());
            ImGui::TableSetupColumn(loc.text("labels.size").data());
            ImGui::TableSetupColumn(loc.text("labels.actions").data());
            ImGui::TableHeadersRow();
 
            for (int i = 0; i < static_cast<int>(m_backups.size()); ++i)
            {
                const auto& entry = m_backups[i];
                ImGui::TableNextRow();
                
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", entry.name.c_str());
                
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", entry.type.c_str());
                
                ImGui::TableSetColumnIndex(2);
                auto ctime = std::chrono::system_clock::to_time_t(entry.created);
                if (const std::tm* tm = std::localtime(&ctime); tm)
                {
                    char buffer[80];
                    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm);
                    ImGui::Text("%s", buffer);
                }
                
                ImGui::TableSetColumnIndex(3);
                if (entry.size < 1024)
                    ImGui::Text("%zu B", entry.size);
                else if (entry.size < 1024 * 1024)
                    ImGui::Text("%.2f KB", entry.size / 1024.0);
                else
                    ImGui::Text("%.2f MB", entry.size / (1024.0 * 1024.0));
 
                ImGui::TableSetColumnIndex(4);
                ImGui::PushID(i);
                if (ImGui::SmallButton(loc.text("buttons.restore").data()))
                {
                    m_pendingBackupIndex = i;
                    m_showRestoreConfirm = true;
                }
                ImGui::SameLine();
                if (ImGui::SmallButton(loc.text("buttons.delete").data()))
                {
                    m_pendingBackupIndex = i;
                    m_showDeleteConfirm = true;
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }

    std::string getTimestampString()
    {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
#if defined(_WIN32)
        localtime_s(&tm, &in_time_t);
#else
        localtime_r(&in_time_t, &tm);
#endif
        char buffer[20];
        std::strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", &tm);
        return std::string(buffer);
    }

    void BackupRestoreWindow::backupAllConfigs()
    {
        try
        {
            fs::path userDir = autoinput::getUserConfigsPath();
            fs::path backupDir = getBackupDirPath() / ("backup_" + getTimestampString() + "_all");
            
            if (!fs::exists(backupDir))
                fs::create_directories(backupDir);

            int count = 0;
            for (const auto& entry : fs::directory_iterator(userDir))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".toml")
                {
                    fs::copy_file(entry.path(), backupDir / entry.path().filename(), fs::copy_options::overwrite_existing);
                    count++;
                }
            }

            m_statusMessage = std::format("Successfully backed up {} configurations.", count);
            m_statusIsError = false;
            Logger::info(m_statusMessage);
            refreshBackups();
        }
        catch (const std::exception& e)
        {
            m_statusMessage = std::format("Backup failed: {}", e.what());
            m_statusIsError = true;
            Logger::error(m_statusMessage);
        }
    }

    void BackupRestoreWindow::backupSelectedConfig(const std::string& configName)
    {
        try
        {
            fs::path configPath = autoinput::getConfigFilePath(configName);
            fs::path backupDir = getBackupDirPath() / ("backup_" + getTimestampString() + "_config_" + configName);
            
            if (!fs::exists(backupDir))
                fs::create_directories(backupDir);

            fs::copy_file(configPath, backupDir / configPath.filename(), fs::copy_options::overwrite_existing);

            m_statusMessage = std::format("Successfully backed up config '{}'.", configName);
            m_statusIsError = false;
            Logger::info(m_statusMessage);
            refreshBackups();
        }
        catch (const std::exception& e)
        {
            m_statusMessage = std::format("Backup failed: {}", e.what());
            m_statusIsError = true;
            Logger::error(m_statusMessage);
        }
    }

    void BackupRestoreWindow::backupSettings()
    {
        try
        {
            fs::path settingsPath = autoinput::getUserConfigsPath() / autoinput::defaults::SettingFileName;
            if (!fs::exists(settingsPath))
            {
                 m_statusMessage = "Settings file does not exist.";
                 m_statusIsError = true;
                 return;
            }

            fs::path backupDir = getBackupDirPath() / ("backup_" + getTimestampString() + "_settings");
            
            if (!fs::exists(backupDir))
                fs::create_directories(backupDir);

            fs::copy_file(settingsPath, backupDir / settingsPath.filename(), fs::copy_options::overwrite_existing);

            m_statusMessage = "Successfully backed up settings.";
            m_statusIsError = false;
            Logger::info(m_statusMessage);
            refreshBackups();
        }
        catch (const std::exception& e)
        {
            m_statusMessage = std::format("Backup failed: {}", e.what());
            m_statusIsError = true;
            Logger::error(m_statusMessage);
        }
    }

    void BackupRestoreWindow::restoreBackup(const BackupEntry& entry)
    {
        try
        {
            fs::path userDir = autoinput::getUserConfigsPath();
            
            for (const auto& subEntry : fs::directory_iterator(entry.path))
            {
                if (subEntry.is_regular_file())
                {
                    fs::copy_file(subEntry.path(), userDir / subEntry.path().filename(), fs::copy_options::overwrite_existing);
                }
            }

            m_statusMessage = std::format("Successfully restored backup '{}'.", entry.name);
            
            // Validate restored configs
            int invalidCount = 0;
            for (const auto& configName : autoinput::listAvailableConfigs())
            {
                auto config = autoinput::loadConfigData(autoinput::getConfigFilePath(configName));
                if (config)
                {
                    auto errors = autoinput::validateConfigData(*config);
                    if (!errors.empty())
                    {
                        invalidCount++;
                    }
                }
            }

            if (invalidCount > 0)
            {
                m_statusMessage += std::format(" Warning: {} configurations have validation errors.", invalidCount);
            }

            m_statusIsError = false;
            Logger::info(m_statusMessage);
            
            // Re-list configs in case they changed
            m_availableConfigs = autoinput::listAvailableConfigs();
        }
        catch (const std::exception& e)
        {
            m_statusMessage = std::format("Restore failed: {}", e.what());
            m_statusIsError = true;
            Logger::error(m_statusMessage);
        }
    }

    void BackupRestoreWindow::deleteBackup(const BackupEntry& entry)
    {
        try
        {
            fs::remove_all(entry.path);
            m_statusMessage = std::format("Successfully deleted backup '{}'.", entry.name);
            m_statusIsError = false;
            Logger::info(m_statusMessage);
            refreshBackups();
        }
        catch (const std::exception& e)
        {
            m_statusMessage = std::format("Delete failed: {}", e.what());
            m_statusIsError = true;
            Logger::error(m_statusMessage);
        }
    }
}
