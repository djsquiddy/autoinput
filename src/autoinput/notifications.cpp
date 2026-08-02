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
        void notify(const std::string& /*title*/, const std::string& body) override
        {
            if (body.find("ACTIVE") != std::string::npos)
            {
                terminal::printStatus("Auto clicking: ", "ACTIVE", terminal::Color::Green);
            }
            else if (body.find("PAUSED") != std::string::npos)
            {
                terminal::printStatus("Auto clicking: ", "PAUSED", terminal::Color::Yellow);
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
            auto desktopSink = platform::createDesktopNotificationSink();
            if (desktopSink)
            {
                m_sinks.push_back(std::move(desktopSink));
            }
        }
    }

    void NotificationService::notifyStatus(bool active)
    {
        if (m_jsonOutput || m_mode == StatusNotificationMode::Off)
        {
            return;
        }

        const std::string title = "AutoInput";
        const std::string body = active ? "Auto clicking: ACTIVE" : "Auto clicking: PAUSED";

        for (const auto& sink : m_sinks)
        {
            sink->notify(title, body);
        }
    }
}
