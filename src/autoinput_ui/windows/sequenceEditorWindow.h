/**
 * @file sequenceEditorWindow.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_WINDOWS_SEQUENCE_EDITOR_WINDOW_H
#define INCLUDE_AUTOINPUT_UI_WINDOWS_SEQUENCE_EDITOR_WINDOW_H
#pragma once

#include "../core/uiWindow.h"
#include "autoinput/config/config.h"
#include <vector>
#include <string>
#include <filesystem>
#include <optional>

namespace autoinput
{
    struct ValidationError;
}

namespace autoinput::ui
{
    /**
     * @brief Window for editing automation sequences.
     */
    class SequenceEditorWindow final : public UiWindow
    {
    public:
        SequenceEditorWindow();

    protected:
        void renderContent() override;
        void save() override;

    private:
        void refreshConfigList();
        void loadConfig(const std::string& nameOrPath);
        void saveConfig(bool forceUser = false);
        void validate();
        void duplicateConfig();

        void normalizeDelays(bool removeZeros);

        autoinput::ConfigData m_configData;
        std::string m_currentConfigName;
        std::filesystem::path m_currentConfigPath;

        int m_selectedSequenceIndex{ -1 };
        std::vector<std::string> m_availableConfigs;
        std::vector<autoinput::ValidationError> m_validationErrors;
        std::string m_statusMessage;

        void renderToolbar();
        void renderSequenceSelector();
        void renderSequenceEditor();
        void renderStepEditor(autoinput::RecordedEvent& event, size_t index);
        
        void insertEvent(autoinput::RecordedEventType type, size_t index);
    };
}

#endif // INCLUDE_AUTOINPUT_UI_WINDOWS_SEQUENCE_EDITOR_WINDOW_H
