/**
 * @file notifications_linux.cpp
 * @author djsquiddy
 * @date August 2026
 */

#include "autoinput/notifications.h"
#include "autoinput/platform.h"
#include "autoinput/logger.h"
#include <string>
#include <cstdlib>
#include <cstdio>
#include <array>

namespace autoinput
{
    namespace
    {
        class LinuxDesktopNotificationSink : public INotificationSink
        {
        public:
            void notify(const std::string& title, const std::string& body, NotificationSeverity severity) override
            {
                if (std::getenv("DBUS_SESSION_BUS_ADDRESS") == nullptr)
                {
                    Logger::debug("DBUS_SESSION_BUS_ADDRESS not set, skipping desktop notification.\n");
                    return;
                }

                // notify-send -p prints the notification ID.
                // notify-send -r <id> replaces the notification with that ID.
                // notify-send -t <ms> sets the timeout.
                // notify-send -a <appname> sets the application name.
                // notify-send -u <urgency> sets the urgency level (low, normal, critical).
                
                std::string urgency = "normal";
                if (severity == NotificationSeverity::Error) urgency = "critical";
                else if (severity == NotificationSeverity::Warning) urgency = "normal";
                else urgency = "low";

                std::string command = "notify-send \"" + title + "\" \"" + body + "\" -t 2000 -a AutoInput -u " + urgency;
                if (m_lastId != 0)
                {
                    command += " -r " + std::to_string(m_lastId);
                }
                command += " -p 2>/dev/null";

                FILE* pipe = popen(command.c_str(), "r");
                if (pipe)
                {
                    std::array<char, 128> buffer;
                    if (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
                    {
                        try
                        {
                            m_lastId = static_cast<unsigned int>(std::stoul(buffer.data()));
                        }
                        catch (...)
                        {
                            m_lastId = 0;
                        }
                    }
                    pclose(pipe);
                }
                else
                {
                    // Fallback to simple notify-send if popen fails
                    std::string fallbackCommand = "notify-send \"" + title + "\" \"" + body + "\" -t 2000 -a AutoInput";
                    if (std::system(fallbackCommand.c_str()) != 0)
                    {
                        Logger::debug("Failed to execute notify-send.\n");
                    }
                }
            }

        private:
            unsigned int m_lastId = 0;
        };
    }

    namespace platform
    {
        std::unique_ptr<INotificationSink> createDesktopNotificationSink()
        {
            return std::make_unique<LinuxDesktopNotificationSink>();
        }
    }
}
