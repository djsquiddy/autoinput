/**
 * @file importExportWindow.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_WINDOWS_IMPORT_EXPORT_WINDOW_H
#define INCLUDE_AUTOINPUT_UI_WINDOWS_IMPORT_EXPORT_WINDOW_H
#pragma once

#include "../core/uiWindow.h"
#include "autoinput/config/config.h"
#include "autoinput/config/configValidator.h"
#include <string>
#include <vector>
#include <optional>

namespace autoinput::ui
{
    /**
     * @brief Window for importing and exporting configurations.
     */
    class ImportExportWindow : public UiWindow
    {
    public:
        ImportExportWindow();

    protected:
        void renderContent() override;
        void onOpen() override;

    private:
        void renderExportSection();
        void renderImportSection();
        
        void refreshAvailableConfigs();
        void handleExport();
        void handleImport();
        void previewImport();

        // Export state
        std::vector<std::string> m_availableConfigs;
        int m_selectedExportIndex = 0;
        std::string m_exportDestPath;
        std::string m_exportStatus;
        bool m_exportSuccess = true;

        // Import state
        std::string m_importSourcePath;
        std::optional<ConfigData> m_importPreview;
        std::vector<ValidationError> m_importValidation;
        std::string m_importStatus;
        bool m_importSuccess = true;
        bool m_validateAfterImport = true;
        
        enum class ConflictResolution
        {
            None,
            Rename,
            Overwrite,
            Cancel
        };
        ConflictResolution m_conflictResolution = ConflictResolution::None;
        bool m_hasConflict = false;
        std::string m_conflictingConfigName;
    };
}

#endif // INCLUDE_AUTOINPUT_UI_WINDOWS_IMPORT_EXPORT_WINDOW_H
