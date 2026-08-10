/**
 * @file importExportWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "importExportWindow.h"
#include "../widgets/basicWidgets.h"
#include "../core/localization.h"
#include "autoinput/config/configValidator.h"
#include "autoinput/support/logger.h"
#include <imgui.h>
#include <imgui_stdlib.h>
#include <filesystem>
#include <format>
#include <algorithm>

namespace autoinput::ui
{
    ImportExportWindow::ImportExportWindow()
        : UiWindow("Import / Export", "windows.importExport")
    {
        refreshAvailableConfigs();
    }

    void ImportExportWindow::onOpen()
    {
        refreshAvailableConfigs();
        m_exportStatus.clear();
        m_importStatus.clear();
        m_importPreview.reset();
        m_importValidation.clear();
        m_hasConflict = false;
        m_conflictResolution = ConflictResolution::None;
    }

    void ImportExportWindow::refreshAvailableConfigs()
    {
        m_availableConfigs = autoinput::listAvailableConfigs();
    }

    void ImportExportWindow::renderContent()
    {
        renderExportSection();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        renderImportSection();
    }

    void ImportExportWindow::renderExportSection()
    {
        auto& loc = Localization::get();
        ImGui::TextDisabled("%s", loc.text("labels.exportConfiguration").data());
        ImGui::Spacing();
 
        if (m_availableConfigs.empty())
        {
            ImGui::Text("%s", loc.text("labels.noConfigsToExport").data());
            return;
        }
 
        if (m_selectedExportIndex >= static_cast<int>(m_availableConfigs.size()))
        {
            m_selectedExportIndex = 0;
        }
 
        const char* preview = m_availableConfigs[m_selectedExportIndex].c_str();
        if (ImGui::BeginCombo(loc.text("labels.selectConfig").data(), preview))
        {
            for (int i = 0; i < static_cast<int>(m_availableConfigs.size()); ++i)
            {
                const bool isSelected = (m_selectedExportIndex == i);
                if (ImGui::Selectable(m_availableConfigs[i].c_str(), isSelected))
                {
                    m_selectedExportIndex = i;
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        std::string sourcePath = autoinput::getConfigFilePath(m_availableConfigs[m_selectedExportIndex]).string();
        ImGui::LabelText(loc.text("labels.sourcePath").data(), "%s", sourcePath.c_str());
 
        ImGui::InputText(loc.text("labels.destinationPath").data(), &m_exportDestPath);
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.export").data()))
        {
            handleExport();
        }

        if (!m_exportStatus.empty())
        {
            if (m_exportSuccess)
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", m_exportStatus.c_str());
            else
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", m_exportStatus.c_str());
        }
    }

    void ImportExportWindow::renderImportSection()
    {
        auto& loc = Localization::get();
        ImGui::TextDisabled("%s", loc.text("labels.importConfiguration").data());
        ImGui::Spacing();
 
        ImGui::InputText(loc.text("labels.sourceFile").data(), &m_importSourcePath);
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.preview").data()))
        {
            previewImport();
        }
 
        if (m_importPreview)
        {
            ImGui::BeginChild("Preview", ImVec2(0, 150), true);
            ImGui::Text("%s: %s", loc.text("labels.previewing").data(), m_importSourcePath.c_str());
            ImGui::BulletText("%s: %zu", loc.text("labels.commands").data(), m_importPreview->commands.size());
            ImGui::BulletText("%s: %zu", loc.text("labels.sequences").data(), m_importPreview->sequences.size());
            
            if (m_importValidation.empty())
            {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s: %s", loc.text("labels.validation").data(), loc.text("buttons.ok").data());
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s: %zu %s", loc.text("labels.validation").data(), m_importValidation.size(), loc.text("labels.issuesFound").data());
                if (ImGui::TreeNode(loc.text("labels.viewErrors").data()))
                {
                    widgets::ValidationErrors(m_importValidation);
                    ImGui::TreePop();
                }
            }
            ImGui::EndChild();
 
            if (m_hasConflict)
            {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", loc.format("labels.importConflictWarning", m_conflictingConfigName).c_str());
                
                ImGui::RadioButton(loc.text("labels.renameImported").data(), reinterpret_cast<int*>(&m_conflictResolution), static_cast<int>(ConflictResolution::Rename));
                ImGui::RadioButton(loc.text("labels.overwriteExisting").data(), reinterpret_cast<int*>(&m_conflictResolution), static_cast<int>(ConflictResolution::Overwrite));
                ImGui::RadioButton(loc.text("buttons.cancel").data(), reinterpret_cast<int*>(&m_conflictResolution), static_cast<int>(ConflictResolution::Cancel));
            }
 
            ImGui::Checkbox(loc.text("labels.validateAfterImport").data(), &m_validateAfterImport);
 
            if (ImGui::Button(loc.text("buttons.import").data()))
            {
                handleImport();
            }
        }

        if (!m_importStatus.empty())
        {
            if (m_importSuccess)
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", m_importStatus.c_str());
            else
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", m_importStatus.c_str());
        }
    }

    void ImportExportWindow::handleExport()
    {
        auto& loc = Localization::get();
        if (m_exportDestPath.empty())
        {
            m_exportStatus = loc.text("status.destPathEmpty");
            m_exportSuccess = false;
            return;
        }
 
        std::filesystem::path destPath(m_exportDestPath);
        if (destPath.extension() != ".toml")
        {
             destPath += ".toml";
        }
 
        auto config = autoinput::loadConfigData(autoinput::getConfigFilePath(m_availableConfigs[m_selectedExportIndex]));
        if (!config)
        {
            m_exportStatus = loc.text("status.failedToLoadSource");
            m_exportSuccess = false;
            return;
        }
 
        if (std::filesystem::exists(destPath))
        {
             // For now, since we don't have a modal helper easily accessible here, we just fail and ask user to use a different path
             // or I could implement a simple confirmation flag.
             m_exportStatus = loc.text("status.destFileExists");
             m_exportSuccess = false;
             return;
        }
 
        if (autoinput::saveConfigData(*config, destPath))
        {
            m_exportStatus = loc.format("status.exportSuccess", destPath.string());
            m_exportSuccess = true;
            Logger::info(m_exportStatus);
        }
        else
        {
            m_exportStatus = loc.text("status.exportFailed");
            m_exportSuccess = false;
        }
    }

    void ImportExportWindow::previewImport()
    {
        auto& loc = Localization::get();
        m_importPreview.reset();
        m_importValidation.clear();
        m_importStatus.clear();
        m_hasConflict = false;
        m_conflictResolution = ConflictResolution::None;
 
        if (m_importSourcePath.empty())
        {
            m_importStatus = loc.text("status.sourcePathEmpty");
            m_importSuccess = false;
            return;
        }
 
        std::filesystem::path sourcePath(m_importSourcePath);
        if (!std::filesystem::exists(sourcePath))
        {
            m_importStatus = loc.text("status.sourceFileMissing");
            m_importSuccess = false;
            return;
        }
 
        auto data = autoinput::loadConfigData(sourcePath);
        if (!data)
        {
            m_importStatus = loc.text("status.importParseFailed");
            m_importSuccess = false;
            return;
        }

        m_importPreview = std::move(data);
        m_importValidation = autoinput::validateConfigData(*m_importPreview);
        
        // Check for conflict
        std::string configName = sourcePath.stem().string();
        std::filesystem::path userPath = autoinput::getUserConfigsPath() / (configName + ".toml");
        
        if (std::filesystem::exists(userPath))
        {
            m_hasConflict = true;
            m_conflictingConfigName = configName;
            m_conflictResolution = ConflictResolution::Rename;
        }
    }

    void ImportExportWindow::handleImport()
    {
        auto& loc = Localization::get();
        if (!m_importPreview)
        {
            m_importStatus = loc.text("status.noImportPreview");
            m_importSuccess = false;
            return;
        }
 
        if (m_hasConflict && m_conflictResolution == ConflictResolution::Cancel)
        {
            m_importStatus = loc.text("status.importCancelled");
            m_importSuccess = false;
            return;
        }
 
        std::filesystem::path sourcePath(m_importSourcePath);
        std::string configName = sourcePath.stem().string();
        
        if (m_hasConflict && m_conflictResolution == ConflictResolution::Rename)
        {
            configName += "_imported";
            // Ensure the new name doesn't conflict either
            int suffix = 1;
            while (std::filesystem::exists(autoinput::getUserConfigsPath() / (configName + std::format("_{}.toml", suffix))))
            {
                suffix++;
            }
            if (suffix > 1) configName += std::format("_{}", suffix);
        }
 
        std::filesystem::path destPath = autoinput::getUserConfigsPath() / (configName + ".toml");
        
        if (autoinput::saveConfigData(*m_importPreview, destPath))
        {
            m_importStatus = loc.format("status.importSuccess", configName);
            m_importSuccess = true;
            Logger::info(m_importStatus);
            
            if (m_validateAfterImport)
            {
                auto errors = autoinput::validateConfigData(*m_importPreview);
                if (!errors.empty())
                {
                    m_importStatus += loc.text("status.importValidationWarning");
                }
            }
            
            refreshAvailableConfigs();
        }
        else
        {
            m_importStatus = loc.text("status.importSaveFailed");
            m_importSuccess = false;
        }
    }
}
