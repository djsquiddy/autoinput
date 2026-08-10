/**
 * @file setupWizardWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "setupWizardWindow.h"
#include "../core/windowManager.h"
#include "../widgets/basicWidgets.h"
#include "../widgets/formWidgets.h"
#include "../core/localization.h"
#include "autoinput/support/logger.h"
#include "autoinput/config/defaults.h"
#include <imgui.h>
#include <imgui_stdlib.h>
#include <format>

namespace autoinput::ui
{
    SetupWizardWindow::SetupWizardWindow(WindowManager& windowManager, 
                                         services::IAutomationRuntimeClient& runtimeClient,
                                         const IEnvironment& environment)
        : UiWindow("Setup Wizard", "windows.setupWizard")
        , m_windowManager(windowManager)
        , m_runtimeClient(runtimeClient)
        , m_environment(environment)
    {
    }

    void SetupWizardWindow::onOpen()
    {
        auto& loc = Localization::get();
        m_currentStep = Step::Welcome;
        m_settings.load();
        
        auto defaults = m_settings.getDefaults();
        m_endKey = defaults.end;
        if (m_endKey.empty()) m_endKey = defaults::EndKey;
        
        m_notificationMode = defaults.statusNotificationMode;
        if (m_notificationMode.empty()) m_notificationMode = defaults::DefaultStatusNotificationMode;
        
        m_uiLanguage = defaults.uiLanguage;
        if (m_uiLanguage.empty()) m_uiLanguage = defaults::DefaultUiLanguage.data();

        m_newConfigName = "default";
        m_newConfig = {};
        m_newConfig.endKey = m_endKey;
        m_newConfig.statusNotificationMode = m_notificationMode;
        
        m_isDiagnosticsPassed = false;
        m_diagnosticsMessage = loc.text("labels.diagnosticsClickTest");
    }

    int SetupWizardWindow::getFlags() const
    {
        return ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
    }

    void SetupWizardWindow::renderContent()
    {
        auto& loc = Localization::get();
        ImGui::Text("%s", loc.format("labels.stepOf", static_cast<int>(m_currentStep) + 1, 6).c_str());
        ImGui::Separator();
        ImGui::Spacing();
 
        switch (m_currentStep)
        {
            case Step::Welcome: renderWelcome(); break;
            case Step::Locations: renderLocations(); break;
            case Step::EndKeyAndNotification: renderEndKeyAndNotification(); break;
            case Step::Diagnostics: renderDiagnostics(); break;
            case Step::FirstConfig: renderFirstConfig(); break;
            case Step::Summary: renderSummary(); break;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        renderButtons();
    }

    void SetupWizardWindow::renderWelcome()
    {
        auto& loc = Localization::get();
        ImGui::Text("%s", loc.text("labels.welcomeTitle").data());
        ImGui::Spacing();
        ImGui::TextWrapped("%s", loc.text("labels.welcomeMessage").data());
    }
 
    void SetupWizardWindow::renderLocations()
    {
        auto& loc = Localization::get();
        ImGui::Text("%s", loc.text("labels.locationsTitle").data());
        ImGui::Spacing();
        
        ImGui::Text("%s:", loc.text("labels.userConfigsPath").data());
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "%s", getUserConfigsPath(m_environment).string().c_str());
        
        ImGui::Spacing();
        
        ImGui::Text("%s:", loc.text("labels.builtinConfigsPath").data());
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "%s", getConfigsPath(m_environment).string().c_str());
        
        ImGui::Spacing();
        ImGui::TextWrapped("%s", loc.text("labels.locationsMessage").data());
    }
 
    void SetupWizardWindow::renderEndKeyAndNotification()
    {
        auto& loc = Localization::get();
        ImGui::Text("%s", loc.text("labels.globalDefaultsTitle").data());
        ImGui::Spacing();
        
        ImGui::TextWrapped("%s", loc.text("labels.endKeyDescription").data());
        ImGui::InputText(loc.text("labels.hotkey").data(), &m_endKey);
        
        ImGui::Spacing();
        
        ImGui::Text("%s:", loc.text("labels.notificationMode").data());
        const char* modes[] = { "console", "toast", "none" };
        int currentMode = 0;
        for (int i = 0; i < 3; ++i)
        {
            if (m_notificationMode == modes[i])
            {
                currentMode = i;
                break;
            }
        }
        
        if (ImGui::Combo("##NotificationMode", &currentMode, modes, IM_ARRAYSIZE(modes)))
        {
            m_notificationMode = modes[currentMode];
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("%s:", loc.text("labels.uiLanguage").data());
        ImGui::TextWrapped("%s", loc.text("labels.restartRequired").data());
        auto availableLanguages = Localization::getAvailableLanguages();
        widgets::StringCombo("##UiLanguage", m_uiLanguage, availableLanguages);
    }
 
    void SetupWizardWindow::renderDiagnostics()
    {
        auto& loc = Localization::get();
        ImGui::Text("%s", loc.text("labels.diagnosticsTitle").data());
        ImGui::Spacing();
        
        ImGui::TextWrapped("%s", loc.text("labels.diagnosticsDescription").data());
        
        ImGui::Spacing();
        
        if (ImGui::Button(loc.text("buttons.runDiagnostics").data()))
        {
            if (m_runtimeClient.ping().success)
            {
                m_isDiagnosticsPassed = true;
                m_diagnosticsMessage = loc.text("labels.diagnosticsPassed");
            }
            else
            {
                m_isDiagnosticsPassed = false;
                m_diagnosticsMessage = loc.text("labels.diagnosticsFailed");
            }
        }
        
        ImGui::Spacing();
        
        if (m_isDiagnosticsPassed)
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", m_diagnosticsMessage.c_str());
        else
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", m_diagnosticsMessage.c_str());
            
        ImGui::Spacing();
        ImGui::Text("%s: %s", loc.text("labels.detectedBackend").data(), m_runtimeClient.getBackendName().c_str());
    }
 
    void SetupWizardWindow::renderFirstConfig()
    {
        auto& loc = Localization::get();
        ImGui::Text("%s", loc.text("labels.firstConfigTitle").data());
        ImGui::Spacing();
        
        ImGui::TextWrapped("%s", loc.text("labels.firstConfigDescription").data());
        
        ImGui::InputText(loc.text("labels.name").data(), &m_newConfigName);
        
        ImGui::Spacing();
        
        ImGui::Text("%s:", loc.text("labels.targetApplicationOptional").data());
        ImGui::InputText(loc.text("labels.process").data(), &m_newConfig.application);
        ImGui::TextDisabled("%s", loc.text("labels.targetApplicationDescription").data());
    }
 
    void SetupWizardWindow::renderSummary()
    {
        auto& loc = Localization::get();
        ImGui::Text("%s", loc.text("labels.setupCompleteTitle").data());
        ImGui::Spacing();
        
        ImGui::TextWrapped("%s", loc.text("labels.setupCompleteMessage").data());
        
        ImGui::Spacing();
        ImGui::Text("%s:", loc.text("labels.suggestedNextSteps").data());
        ImGui::BulletText("%s", loc.text("labels.stepRecordFirst").data());
        ImGui::BulletText("%s", loc.text("labels.stepExecuteCommands").data());
        ImGui::BulletText("%s", loc.text("labels.stepManageFiles").data());
    }

    void SetupWizardWindow::renderButtons()
    {
        auto& loc = Localization::get();
        if (m_currentStep != Step::Welcome)
        {
            if (ImGui::Button(loc.text("buttons.back").data()))
            {
                m_currentStep = static_cast<Step>(static_cast<int>(m_currentStep) - 1);
            }
            ImGui::SameLine();
        }
        
        if (m_currentStep != Step::Summary)
        {
            if (ImGui::Button(loc.text("buttons.next").data()) && validateCurrentStep())
            {
                m_currentStep = static_cast<Step>(static_cast<int>(m_currentStep) + 1);
            }
        }
        else
        {
            if (ImGui::Button(loc.text("buttons.finish").data()))
            {
                saveAndFinish();
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.cancel").data()))
        {
            requestClose();
        }
    }

    bool SetupWizardWindow::validateCurrentStep()
    {
        if (m_currentStep == Step::EndKeyAndNotification)
        {
            if (m_endKey.empty()) return false;
        }
        if (m_currentStep == Step::FirstConfig)
        {
            if (m_newConfigName.empty()) return false;
        }
        return true;
    }

    void SetupWizardWindow::saveAndFinish()
    {
        auto defaults = m_settings.getDefaults();
        defaults.end = m_endKey;
        defaults.statusNotificationMode = m_notificationMode;
        defaults.uiLanguage = m_uiLanguage;
        defaults.setupCompleted = true;
        m_settings.setDefaults(defaults);
        
        if (saveUserSettings(m_settings))
        {
            Logger::info("Setup wizard: Settings saved.");
        }
        else
        {
            Logger::error("Setup wizard: Failed to save settings.");
        }
        
        // Save first config
        m_newConfig.endKey = m_endKey;
        m_newConfig.statusNotificationMode = m_notificationMode;

        if (const auto configPath = getUserConfigsPath(m_environment) / (m_newConfigName + ".toml");
            saveConfigData(m_newConfig, configPath, defaults))
        {
            Logger::info("Setup wizard: Initial config '{}' saved.", m_newConfigName);
        }
        
        requestClose();
    }
}
