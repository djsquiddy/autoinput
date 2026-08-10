/**
 * @file setupWizardWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "setupWizardWindow.h"
#include "../core/windowManager.h"
#include "../widgets/basicWidgets.h"
#include "autoinput/logger.h"
#include "autoinput/defaults.h"
#include <imgui.h>
#include <imgui_stdlib.h>
#include <format>

namespace autoinput::ui
{
    SetupWizardWindow::SetupWizardWindow(WindowManager& windowManager, 
                                         services::IAutomationRuntimeClient& runtimeClient,
                                         const IEnvironment& environment)
        : UiWindow("Setup Wizard")
        , m_windowManager(windowManager)
        , m_runtimeClient(runtimeClient)
        , m_environment(environment)
    {
    }

    void SetupWizardWindow::onOpen()
    {
        m_currentStep = Step::Welcome;
        m_settings.load();
        
        auto defaults = m_settings.getDefaults();
        m_endKey = defaults.end;
        if (m_endKey.empty()) m_endKey = defaults::EndKey;
        
        m_notificationMode = defaults.statusNotificationMode;
        if (m_notificationMode.empty()) m_notificationMode = defaults::DefaultStatusNotificationMode;
        
        m_newConfigName = "default";
        m_newConfig = {};
        m_newConfig.endKey = m_endKey;
        m_newConfig.statusNotificationMode = m_notificationMode;
        
        m_isDiagnosticsPassed = false;
        m_diagnosticsMessage = "Click 'Test Connection' to run diagnostics.";
    }

    int SetupWizardWindow::getFlags() const
    {
        return ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
    }

    void SetupWizardWindow::renderContent()
    {
        ImGui::Text("Step %d of 6", static_cast<int>(m_currentStep) + 1);
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
        ImGui::Text("Welcome to AutoInput!");
        ImGui::Spacing();
        ImGui::TextWrapped("This wizard will help you configure the initial settings for AutoInput "
                           "so you can start automating your workflow quickly.");
        ImGui::Spacing();
        ImGui::TextWrapped("We'll guide you through setting up file locations, hotkeys, and testing "
                           "your backend compatibility.");
    }

    void SetupWizardWindow::renderLocations()
    {
        ImGui::Text("Config & Settings Locations");
        ImGui::Spacing();
        
        ImGui::Text("User Configs Path:");
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "%s", getUserConfigsPath(m_environment).string().c_str());
        
        ImGui::Spacing();
        
        ImGui::Text("Built-in Configs Path:");
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "%s", getConfigsPath(m_environment).string().c_str());
        
        ImGui::Spacing();
        ImGui::TextWrapped("Configurations created by you will be stored in the User Configs Path.");
    }

    void SetupWizardWindow::renderEndKeyAndNotification()
    {
        ImGui::Text("Global Defaults");
        ImGui::Spacing();
        
        ImGui::TextWrapped("Choose a global 'End Key' that will stop any running automation. "
                           "This acts as an emergency stop.");
        ImGui::InputText("End Key", &m_endKey);
        
        ImGui::Spacing();
        
        ImGui::Text("Status Notification Mode:");
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
        
        if (ImGui::Combo("Notification Mode", &currentMode, modes, IM_ARRAYSIZE(modes)))
        {
            m_notificationMode = modes[currentMode];
        }
    }

    void SetupWizardWindow::renderDiagnostics()
    {
        ImGui::Text("Backend & Permission Diagnostics");
        ImGui::Spacing();
        
        ImGui::TextWrapped("AutoInput needs appropriate permissions to simulate input. "
                           "Let's test if the runtime and backend are working correctly.");
        
        ImGui::Spacing();
        
        if (ImGui::Button("Run Diagnostics"))
        {
            if (m_runtimeClient.ping().success)
            {
                m_isDiagnosticsPassed = true;
                m_diagnosticsMessage = "Diagnostics passed! Backend is reachable and responsive.";
            }
            else
            {
                m_isDiagnosticsPassed = false;
                m_diagnosticsMessage = "Diagnostics failed. Please ensure the runtime is running and has permissions.";
            }
        }
        
        ImGui::Spacing();
        
        if (m_isDiagnosticsPassed)
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", m_diagnosticsMessage.c_str());
        else
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", m_diagnosticsMessage.c_str());
            
        ImGui::Spacing();
        ImGui::Text("Detected Backend: %s", m_runtimeClient.getBackendName().c_str());
    }

    void SetupWizardWindow::renderFirstConfig()
    {
        ImGui::Text("Create Your First Config");
        ImGui::Spacing();
        
        ImGui::TextWrapped("Enter a name for your first configuration file. "
                           "You can add commands and sequences to it later.");
        
        ImGui::InputText("Config Name", &m_newConfigName);
        
        ImGui::Spacing();
        
        ImGui::Text("Target Application (Optional):");
        ImGui::InputText("Application", &m_newConfig.application);
        ImGui::TextDisabled("If set, this config will only active when this app is in focus.");
    }

    void SetupWizardWindow::renderSummary()
    {
        ImGui::Text("Setup Complete!");
        ImGui::Spacing();
        
        ImGui::TextWrapped("You have successfully configured AutoInput. Click 'Finish' to save "
                           "your settings and start using the application.");
        
        ImGui::Spacing();
        ImGui::Text("Suggested next steps:");
        ImGui::BulletText("Open 'Sequence Recorder' to record your first automation.");
        ImGui::BulletText("Use 'Command Runner' to execute specific commands.");
        ImGui::BulletText("Manage your files in 'Config Manager'.");
    }

    void SetupWizardWindow::renderButtons()
    {
        if (m_currentStep != Step::Welcome)
        {
            if (ImGui::Button("Back"))
            {
                m_currentStep = static_cast<Step>(static_cast<int>(m_currentStep) - 1);
            }
            ImGui::SameLine();
        }
        
        if (m_currentStep != Step::Summary)
        {
            if (ImGui::Button("Next") && validateCurrentStep())
            {
                m_currentStep = static_cast<Step>(static_cast<int>(m_currentStep) + 1);
            }
        }
        else
        {
            if (ImGui::Button("Finish"))
            {
                saveAndFinish();
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
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
