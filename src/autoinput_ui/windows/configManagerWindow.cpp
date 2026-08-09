/**
 * @file configManagerWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "configManagerWindow.h"
#include "../widgets/basicWidgets.h"
#include "../widgets/formWidgets.h"
#include "autoinput/logger.h"
#include "autoinput/configValidator.h"
#include "configEditorWindow.h"
#include "../core/windowManager.h"
#include <imgui.h>
#include <imgui_stdlib.h>
#include <format>
#include <chrono>

namespace autoinput::ui
{
    ConfigManagerWindow::ConfigManagerWindow(WindowManager& windowManager, const IEnvironment& environment)
        : UiWindow("Config Manager")
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
            
            try {
                auto ftime = std::filesystem::last_write_time(entry.path);
                auto sctp = std::chrono::file_clock::to_sys(ftime);
                auto ctime = std::chrono::system_clock::to_time_t(sctp);
                std::tm* tm = std::localtime(&ctime);
                if (tm)
                {
                    char buffer[80];
                    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm);
                    entry.lastModified = buffer;
                }
                else
                {
                    entry.lastModified = "Unknown";
                }
            } catch (...) {
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
        m_statusMessage = "Validated all configurations.";
    }

    void ConfigManagerWindow::deleteConfig(const ConfigEntry& entry)
    {
        if (entry.type == ConfigType::Global)
        {
            m_statusMessage = "Cannot delete built-in configs. Duplicate to User first.";
            return;
        }

        try {
            if (std::filesystem::remove(entry.path))
            {
                m_statusMessage = std::format("Deleted config: {}", entry.name);
                refreshConfigs();
            }
            else
            {
                m_statusMessage = std::format("Failed to delete config: {}", entry.name);
            }
        } catch (const std::exception& e) {
            m_statusMessage = std::format("Error deleting config: {}", e.what());
        }
    }

    void ConfigManagerWindow::duplicateConfig(const ConfigEntry& entry)
    {
        std::string newName = entry.name + "_copy";
        std::filesystem::path destPath = autoinput::getUserConfigsPath(m_environment) / (newName + ".toml");
        
        if (autoinput::duplicateConfig(entry.path.string(), destPath.string(), false))
        {
            m_statusMessage = std::format("Duplicated {} to {}", entry.name, newName);
            refreshConfigs();
        }
        else
        {
            m_statusMessage = std::format("Failed to duplicate config: {}", entry.name);
        }
    }

    void ConfigManagerWindow::renameConfig(const ConfigEntry& entry, const std::string& newName)
    {
        if (entry.type == ConfigType::Global)
        {
            m_statusMessage = "Cannot rename built-in configs. Duplicate to User first.";
            return;
        }

        std::filesystem::path newPath = entry.path.parent_path() / (newName + ".toml");
        try {
            std::filesystem::rename(entry.path, newPath);
            m_statusMessage = std::format("Renamed {} to {}", entry.name, newName);
            refreshConfigs();
        } catch (const std::exception& e) {
            m_statusMessage = std::format("Error renaming config: {}", e.what());
        }
    }

    void ConfigManagerWindow::createNewConfig(const std::string& name)
    {
        std::filesystem::path path = autoinput::getUserConfigsPath(m_environment) / (name + ".toml");
        if (std::filesystem::exists(path))
        {
            m_statusMessage = std::format("Config {} already exists.", name);
            return;
        }

        ConfigData data;
        if (autoinput::saveConfigData(data, path))
        {
            m_statusMessage = std::format("Created new config: {}", name);
            refreshConfigs();
        }
        else
        {
            m_statusMessage = std::format("Failed to create config: {}", name);
        }
    }

    void ConfigManagerWindow::importConfig(const std::filesystem::path& sourcePath)
    {
        std::string name = sourcePath.stem().string();
        std::filesystem::path destPath = autoinput::getUserConfigsPath(m_environment) / (name + ".toml");
        
        try {
            std::filesystem::copy_file(sourcePath, destPath, std::filesystem::copy_options::overwrite_existing);
            m_statusMessage = std::format("Imported config: {}", name);
            refreshConfigs();
        } catch (const std::exception& e) {
            m_statusMessage = std::format("Error importing config: {}", e.what());
        }
    }

    void ConfigManagerWindow::exportConfig(const ConfigEntry& entry, const std::filesystem::path& destPath)
    {
        try {
            std::filesystem::copy_file(entry.path, destPath, std::filesystem::copy_options::overwrite_existing);
            m_statusMessage = std::format("Exported {} to {}", entry.name, destPath.string());
        } catch (const std::exception& e) {
            m_statusMessage = std::format("Error exporting config: {}", e.what());
        }
    }

    void ConfigManagerWindow::renderContent()
    {
        if (ImGui::Button("Refresh")) refreshConfigs();
        ImGui::SameLine();
        if (ImGui::Button("New Config"))
        {
            m_newConfigNameBuffer = "new_config";
            m_showNewConfigModal = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Validate All")) validateAll();
        ImGui::SameLine();
        if (ImGui::Button("Import"))
        {
             m_statusMessage = "Import: Feature coming soon. Copy files to configs folder manually.";
        }

        ImGui::Separator();

        if (ImGui::BeginTable("ConfigsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY, ImVec2(0, 300)))
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Source");
            ImGui::TableSetupColumn("Status");
            ImGui::TableSetupColumn("Last Modified");
            ImGui::TableSetupColumn("Path");
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
                    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Valid");
                else if (!entry.validationErrors.empty())
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "Invalid");
                else
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1), "Unknown");

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
            
            if (ImGui::Button("Open in Editor"))
            {
                if (auto editor = m_windowManager.findAs<ConfigEditorWindow>("config-editor"))
                {
                    editor->loadConfig(selected.path.string());
                    m_windowManager.open("config-editor");
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Validate")) validateConfig(selected);
            ImGui::SameLine();
            if (ImGui::Button("Duplicate")) duplicateConfig(selected);
            
            ImGui::SameLine();
            if (selected.type == ConfigType::User)
            {
                if (ImGui::Button("Rename"))
                {
                    m_newNameBuffer = selected.name;
                    m_configToOperate = selected;
                    m_showRenameModal = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Delete"))
                {
                    m_configToOperate = selected;
                    m_showDeleteConfirm = true;
                }
            }
            else
            {
                ImGui::BeginDisabled();
                ImGui::Button("Rename");
                ImGui::SameLine();
                ImGui::Button("Delete");
                ImGui::EndDisabled();
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Cannot modify built-in configs.");
            }

            ImGui::SameLine();
            if (ImGui::Button("Open Folder"))
            {
                m_environment.openPath(selected.path.parent_path());
            }
            
            if (!selected.validationErrors.empty())
            {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Errors:");
                for (const auto& err : selected.validationErrors)
                {
                    ImGui::BulletText("%s", err.message.c_str());
                }
            }
        }
        else
        {
            ImGui::TextDisabled("Select a configuration from the table to see options.");
        }

        if (!m_statusMessage.empty())
        {
            ImGui::Spacing();
            widgets::StatusText(m_statusMessage);
        }

        // Modals
        if (m_showDeleteConfirm)
        {
            ImGui::OpenPopup("Delete Config?");
            m_showDeleteConfirm = false;
        }

        if (ImGui::BeginPopupModal("Delete Config?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Are you sure you want to delete '%s'?\nThis operation cannot be undone.", m_configToOperate.name.c_str());
            ImGui::Separator();

            if (ImGui::Button("Delete", ImVec2(120, 0)))
            {
                deleteConfig(m_configToOperate);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }

        if (m_showRenameModal)
        {
            ImGui::OpenPopup("Rename Config");
            m_showRenameModal = false;
        }

        if (ImGui::BeginPopupModal("Rename Config", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Enter new name for '%s':", m_configToOperate.name.c_str());
            ImGui::InputText("##newname", &m_newNameBuffer);
            ImGui::Separator();

            if (ImGui::Button("Rename", ImVec2(120, 0)))
            {
                renameConfig(m_configToOperate, m_newNameBuffer);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }

        if (m_showNewConfigModal)
        {
            ImGui::OpenPopup("New Config");
            m_showNewConfigModal = false;
        }

        if (ImGui::BeginPopupModal("New Config", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Enter name for new config:");
            ImGui::InputText("##configname", &m_newConfigNameBuffer);
            ImGui::Separator();

            if (ImGui::Button("Create", ImVec2(120, 0)))
            {
                createNewConfig(m_newConfigNameBuffer);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
    }
}
