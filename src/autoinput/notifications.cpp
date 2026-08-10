/**
 * @file notifications.cpp
 * @author djsquiddy
 * @date August 2026
 */

#include "autoinput/notifications.h"
#include "autoinput/terminal.h"
#include "autoinput/platform.h"

namespace autoinput
{
    class ConsoleNotificationSink : public INotificationSink
    {
    public:
        void notify(const std::string& /*title*/, const std::string& body, NotificationSeverity /*severity*/) override
        {
            if (body.find("ACTIVE") != std::string::npos)
            {
                const auto pos = body.find(" (");
                const auto endPos = body.find("):");
                if (pos != std::string::npos && endPos != std::string::npos)
                {
                    const std::string name = body.substr(pos + 2, endPos - pos - 2);
                    terminal::printStatus(std::format("Auto clicking ({}): ", name), "ACTIVE", terminal::Color::Green);
                }
                else
                {
                    terminal::printStatus("Auto clicking: ", "ACTIVE", terminal::Color::Green);
                }
            }
            else if (body.find("PAUSED") != std::string::npos)
            {
                const auto pos = body.find(" (");
                const auto endPos = body.find("):");
                if (pos != std::string::npos && endPos != std::string::npos)
                {
                    const std::string name = body.substr(pos + 2, endPos - pos - 2);
                    terminal::printStatus(std::format("Auto clicking ({}): ", name), "PAUSED", terminal::Color::Yellow);
                }
                else
                {
                    terminal::printStatus("Auto clicking: ", "PAUSED", terminal::Color::Yellow);
                }
            }
        }
    };

    NotificationService::NotificationService(StatusNotificationMode mode, bool jsonOutput)
        : m_mode(mode), m_jsonOutput(jsonOutput)
    {
        if (m_jsonOutput || m_mode == StatusNotificationMode::Off)
        {
            return;
        }

        if ((m_mode & StatusNotificationMode::Console) != StatusNotificationMode::Off)
        {
            m_sinks.push_back(std::make_unique<ConsoleNotificationSink>());
        }

        if ((m_mode & StatusNotificationMode::Desktop) != StatusNotificationMode::Off)
        {
            if (auto desktopSink = platform::createDesktopNotificationSink())
            {
                m_sinks.push_back(std::move(desktopSink));
            }
        }
    }

    void NotificationService::notifyStatus(const bool active, const std::string& commandName, std::optional<bool> commandActive)
    {
        if (m_jsonOutput || m_mode == StatusNotificationMode::Off)
        {
            return;
        }

        const bool displayActive = commandActive.value_or(active);
        const std::string title = "AutoInput";
        std::string body;
        if (commandName.empty())
        {
            body = displayActive ? "Auto clicking: ACTIVE" : "Auto clicking: PAUSED";
        }
        else
        {
            body = std::format("Auto clicking ({}): {}", commandName, displayActive ? "ACTIVE" : "PAUSED");
        }

        for (const auto& sink : m_sinks)
        {
            sink->notify(title, body, displayActive ? NotificationSeverity::Info : NotificationSeverity::Warning);
        }
    }

    void NotificationService::notify(const std::string& title, const std::string& body, NotificationSeverity severity)
    {
        if (m_jsonOutput || m_mode == StatusNotificationMode::Off)
        {
            return;
        }

        for (const auto& sink : m_sinks)
        {
            sink->notify(title, body, severity);
        }
    }
}
