/**
 * @file configManagerWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "configManagerWindow.h"
#include "../widgets/basicWidgets.h"
#include "../widgets/formWidgets.h"
#include "../core/windowIds.h"
#include "../core/localization.h"
#include "autoinput/support/logger.h"
#include "autoinput/config/configValidator.h"
#include "configEditorWindow.h"
#include "../core/windowManager.h"
#include <imgui.h>
#include <imgui_stdlib.h>
#include <format>
#include <chrono>

namespace autoinput::ui
{
    ConfigManagerWindow::ConfigManagerWindow(WindowManager& windowManager, const IEnvironment& environment)
        : UiWindow("Config Manager", "windows.configManager")
        , m_windowManager(windowManager)
        , m_environment(environment)
        , m_configService(environment)
    {
        refreshConfigs();
    }

    void ConfigManagerWindow::refreshConfigs()
    {
        m_configs.clear();
        auto infos = m_configService.listAvailableConfigs();
        for (const auto& info : infos)
        {
            ConfigEntry entry;
            entry.name = info.fileStem();
            entry.type = info.type;
            entry.path = info.filepath;
            
            try
            {
                auto ftime = std::filesystem::last_write_time(entry.path);
                auto sctp = std::chrono::clock_cast<std::chrono::system_clock>(ftime);
                auto ctime = std::chrono::system_clock::to_time_t(sctp);
                if (const std::tm* tm = std::localtime(&ctime); tm)
                {
                    char buffer[80];
                    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm);
                    entry.lastModified = buffer;
                }
                else
                {
                    entry.lastModified = "Unknown";
                }
            }
            catch (...)
            {
                entry.lastModified = "Unknown";
            }
            
            m_configs.push_back(std::move(entry));
        }
        m_selectedIndex = -1;
    }

    void ConfigManagerWindow::validateConfig(ConfigEntry& entry)
    {
        auto result = m_configService.validateConfig(entry.path.string());
        entry.isValid = result.isValid;
        entry.validationErrors = std::move(result.errors);
    }

    void ConfigManagerWindow::validateAll()
    {
        for (auto& entry : m_configs)
        {
            validateConfig(entry);
        }
        m_statusMessage = Localization::get().text("status.validatedAllConfigsMessage");
    }

    void ConfigManagerWindow::deleteConfig(const ConfigEntry& entry)
    {
        if (entry.type == ConfigType::Global)
        {
            m_statusMessage = Localization::get().text("status.cannotDeleteBuiltin");
            return;
        }

        try
        {
            if (std::filesystem::remove(entry.path))
            {
                m_statusMessage = Localization::get().format("status.deletedConfig", entry.name);
                refreshConfigs();
            }
            else
            {
                m_statusMessage = Localization::get().format("status.failedToDeleteConfig", entry.name);
            }
        }
        catch (const std::exception& e)
        {
            m_statusMessage = Localization::get().format("status.errorDeletingConfig", e.what());
        }
    }

    void ConfigManagerWindow::duplicateConfig(const ConfigEntry& entry)
    {
        std::string newName = entry.name + "_copy";
        std::filesystem::path destPath = autoinput::getUserConfigsPath(m_environment) / (newName + ".toml");
        
        int suffix = 1;
        while (std::filesystem::exists(destPath))
        {
            newName = entry.name + std::format("_copy_{}", suffix++);
            destPath = autoinput::getUserConfigsPath(m_environment) / (newName + ".toml");
        }
        
        if (autoinput::duplicateConfig(entry.path.string(), destPath.string(), false))
        {
            m_statusMessage = Localization::get().format("status.duplicatedConfig", entry.name, newName);
            refreshConfigs();
        }
        else
        {
            m_statusMessage = Localization::get().format("status.failedToDuplicateConfig", entry.name);
        }
    }

    void ConfigManagerWindow::renameConfig(const ConfigEntry& entry, const std::string& newName)
    {
        if (entry.type == ConfigType::Global)
        {
            m_statusMessage = Localization::get().text("status.cannotRenameBuiltin");
            return;
        }
 
        const std::filesystem::path newPath = entry.path.parent_path() / (newName + ".toml");
        if (std::filesystem::exists(newPath))
        {
            m_statusMessage = Localization::get().format("status.configAlreadyExists", newName);
            return;
        }
 
        try
        {
            std::filesystem::rename(entry.path, newPath);
            m_statusMessage = Localization::get().format("status.renamedConfig", entry.name, newName);
            refreshConfigs();
        }
        catch (const std::exception& e)
        {
            m_statusMessage = Localization::get().format("status.errorRenamingConfig", e.what());
        }
    }

    void ConfigManagerWindow::createNewConfig(const std::string& name)
    {
        const std::filesystem::path path = getUserConfigsPath(m_environment) / (name + ".toml");
        if (std::filesystem::exists(path))
        {
            m_statusMessage = Localization::get().format("status.configAlreadyExists", name);
            return;
        }
 
        if (const ConfigData data; saveConfigData(data, path))
        {
            m_statusMessage = Localization::get().format("status.createdNewConfig", name);
            refreshConfigs();
        }
        else
        {
            m_statusMessage = Localization::get().format("status.failedToCreateConfig", name);
        }
    }

    void ConfigManagerWindow::importConfig(const std::filesystem::path& sourcePath)
    {
        std::string name = sourcePath.stem().string();
        const std::filesystem::path destPath = getUserConfigsPath(m_environment) / (name + ".toml");
        
        if (std::filesystem::exists(destPath))
        {
            m_statusMessage = std::format("Error: Config '{}' already exists in user directory.", name);
            return;
        }

        try
        {
            std::filesystem::copy_file(sourcePath, destPath, std::filesystem::copy_options::none);
            m_statusMessage = std::format("Imported config: {}", name);
            refreshConfigs();
        }
        catch (const std::exception& e)
        {
            m_statusMessage = std::format("Error importing config: {}", e.what());
        }
    }

    void ConfigManagerWindow::exportConfig(const ConfigEntry& entry, const std::filesystem::path& destPath)
    {
        if (std::filesystem::exists(destPath))
        {
            m_statusMessage = std::format("Error: Destination '{}' already exists.", destPath.filename().string());
            return;
        }

        try
        {
            std::filesystem::copy_file(entry.path, destPath, std::filesystem::copy_options::none);
            m_statusMessage = std::format("Exported {} to {}", entry.name, destPath.string());
        }
        catch (const std::exception& e)
        {
            m_statusMessage = std::format("Error exporting config: {}", e.what());
        }
    }

    void ConfigManagerWindow::renderContent()
    {
        auto& loc = Localization::get();
        if (ImGui::Button(loc.text("buttons.refresh").data())) refreshConfigs();
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.new").data()))
        {
            m_newConfigNameBuffer = "new_config";
            m_showNewConfigModal = true;
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.text("actions.validateAll").data())) validateAll();
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.import").data()))
        {
             m_statusMessage = loc.text("status.importFeatureSoon");
        }

        ImGui::Separator();

        if (ImGui::BeginTable("ConfigsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY, ImVec2(0, 300)))
        {
            ImGui::TableSetupColumn(loc.text("labels.name").data());
            ImGui::TableSetupColumn(loc.text("labels.source").data());
            ImGui::TableSetupColumn(loc.text("labels.status").data());
            ImGui::TableSetupColumn(loc.text("labels.lastModified").data());
            ImGui::TableSetupColumn(loc.text("labels.path").data());
            ImGui::TableHeadersRow();

            for (int i = 0; i < static_cast<int>(m_configs.size()); ++i)
            {
                auto& entry = m_configs[i];
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                
                bool isSelected = (m_selectedIndex == i);
                if (ImGui::Selectable(entry.name.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
                {
                    m_selectedIndex = i;
                }
                
                ImGui::TableNextColumn();
                ImGui::Text("%s", configTypeToString(entry.type).data());
                
                ImGui::TableNextColumn();
                if (entry.validationErrors.empty() && entry.isValid)
                    ImGui::TextColored(ImVec4(0, 1, 0, 1), "%s", loc.text("labels.valid").data());
                else if (!entry.validationErrors.empty())
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", loc.text("labels.invalid").data());
                else
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1), "%s", loc.text("status.unknown").data());

                ImGui::TableNextColumn();
                ImGui::Text("%s", entry.lastModified.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", entry.path.string().c_str());
            }
            ImGui::EndTable();
        }

        ImGui::Separator();

        if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_configs.size()))
        {
            auto& selected = m_configs[m_selectedIndex];
            ImGui::Text("Selected: %s", selected.name.c_str());
            
            if (ImGui::Button(loc.text("buttons.openInEditor").data()))
            {
                if (auto editor = m_windowManager.findAs<ConfigEditorWindow>(WindowIds::ConfigEditor))
                {
                    editor->loadConfig(selected.path.string());
                    m_windowManager.open(WindowIds::ConfigEditor);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(loc.text("buttons.validate").data())) validateConfig(selected);
            ImGui::SameLine();
            if (ImGui::Button(loc.text("buttons.duplicate").data())) duplicateConfig(selected);
            
            ImGui::SameLine();
            if (selected.type == ConfigType::User)
            {
                if (ImGui::Button(loc.text("buttons.rename").data()))
                {
                    m_newNameBuffer = selected.name;
                    m_configToOperate = selected;
                    m_showRenameModal = true;
                }
                ImGui::SameLine();
                if (ImGui::Button(loc.text("buttons.delete").data()))
                {
                    m_configToOperate = selected;
                    m_showDeleteConfirm = true;
                }
            }
            else
            {
                ImGui::BeginDisabled();
                ImGui::Button(loc.text("buttons.rename").data());
                ImGui::SameLine();
                ImGui::Button(loc.text("buttons.delete").data());
                ImGui::EndDisabled();
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", loc.text("status.cannotModifyBuiltin").data());
            }
            ImGui::SameLine();
            if (ImGui::Button(loc.text("buttons.openFolder").data()))
            {
                m_environment.openPath(selected.path.parent_path());
            }
            
            if (!selected.validationErrors.empty())
            {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", loc.text("labels.errorsColon").data());
                for (const auto& err : selected.validationErrors)
                {
                    ImGui::BulletText("%s", err.message.c_str());
                }
            }
        }
        else
        {
            ImGui::TextDisabled("%s", loc.text("status.selectConfigToSeeOptions").data());
        }

        if (!m_statusMessage.empty())
        {
            ImGui::Spacing();
            widgets::StatusText(m_statusMessage);
        }

        // Modals
        if (m_showDeleteConfirm)
        {
            ImGui::OpenPopup(loc.text("modals.deleteConfigTitle").data());
            m_showDeleteConfirm = false;
        }

        if (ImGui::BeginPopupModal(loc.text("modals.deleteConfigTitle").data(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("%s", loc.format("modals.deleteConfigMessage", m_configToOperate.name).c_str());
            ImGui::Separator();

            if (ImGui::Button(loc.text("buttons.delete").data(), ImVec2(120, 0)))
            {
                deleteConfig(m_configToOperate);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button(loc.text("buttons.cancel").data(), ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }

        if (m_showRenameModal)
        {
            ImGui::OpenPopup(loc.text("modals.renameConfigTitle").data());
            m_showRenameModal = false;
        }

        if (ImGui::BeginPopupModal(loc.text("modals.renameConfigTitle").data(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("%s", loc.format("modals.renameConfigMessage", m_configToOperate.name).c_str());
            ImGui::InputText("##newname", &m_newNameBuffer);
            ImGui::Separator();

            if (ImGui::Button(loc.text("buttons.rename").data(), ImVec2(120, 0)))
            {
                renameConfig(m_configToOperate, m_newNameBuffer);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(loc.text("buttons.cancel").data(), ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }

        if (m_showNewConfigModal)
        {
            ImGui::OpenPopup(loc.text("modals.newConfigTitle").data());
            m_showNewConfigModal = false;
        }

        if (ImGui::BeginPopupModal(loc.text("modals.newConfigTitle").data(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("%s", loc.text("modals.newConfigMessage").data());
            ImGui::InputText("##configname", &m_newConfigNameBuffer);
            ImGui::Separator();

            if (ImGui::Button(loc.text("buttons.create").data(), ImVec2(120, 0)))
            {
                createNewConfig(m_newConfigNameBuffer);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(loc.text("buttons.cancel").data(), ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
    }
}
