/**
 * @file backupRestoreWindow.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_WINDOWS_BACKUP_RESTORE_WINDOW_H
#define INCLUDE_AUTOINPUT_UI_WINDOWS_BACKUP_RESTORE_WINDOW_H
#pragma once

#include "../core/uiWindow.h"
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>

namespace autoinput::ui
{
    /**
     * @brief Window for managing backups and restores of configurations and settings.
     */
    class BackupRestoreWindow : public UiWindow
    {
    public:
        BackupRestoreWindow();

    protected:
        void renderContent() override;
        void onOpen() override;

    private:
        struct BackupEntry
        {
            std::string name;
            std::string type; // "Config", "All", "Settings"
            std::chrono::system_clock::time_point created;
            uintmax_t size;
            std::filesystem::path path;
        };

        void refreshBackups();
        void renderBackupTable();
        void renderControls();
        
        void backupAllConfigs();
        void backupSelectedConfig(const std::string& configName);
        void backupSettings();
        
        void restoreBackup(const BackupEntry& entry);
        void deleteBackup(const BackupEntry& entry);
        
        std::filesystem::path getBackupDirPath() const;

        std::vector<BackupEntry> m_backups;
        std::vector<std::string> m_availableConfigs;
        int m_selectedConfigIndex = 0;
        
        std::string m_statusMessage;
        bool m_statusIsError = false;
        
        // Modal state
        bool m_showRestoreConfirm = false;
        bool m_showDeleteConfirm = false;
        int m_pendingBackupIndex = -1;
    };
}

#endif // INCLUDE_AUTOINPUT_UI_WINDOWS_BACKUP_RESTORE_WINDOW_H
