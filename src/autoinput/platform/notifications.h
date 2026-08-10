/**
 * @file notifications.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_PLATFORM_NOTIFICATIONS_H
#define INCLUDE_AUTOINPUT_PLATFORM_NOTIFICATIONS_H
#pragma once

#include <string>
#include <memory>
#include <vector>
#include <optional>
#include "autoinput/support/types.h"

namespace autoinput
{
    class INotificationSink
    {
    public:
        /**
         * @brief Virtual destructor for INotificationSink.
         */
        virtual ~INotificationSink() = default;

        /**
         * @brief Sends a notification.
         * @param title The title of the notification.
         * @param body The body text of the notification.
         * @param severity The severity of the notification.
         */
        virtual void notify(const std::string& title, const std::string& body, NotificationSeverity severity = NotificationSeverity::Info) = 0;
    };

    class NotificationService
    {
    public:
        /**
         * @brief Constructs a NotificationService.
         * @param mode The notification mode (e.g. Console, SystemTray).
         * @param jsonOutput Whether to output notifications as JSON.
         */
        NotificationService(StatusNotificationMode mode, bool jsonOutput);
        
        /**
         * @brief Notifies the status of the program or a command.
         * @param active Whether the program is active.
         * @param commandName The name of the triggered command (optional).
         * @param commandActive Whether the triggered command is active (optional).
         */
        void notifyStatus(bool active, const std::string& commandName = "", std::optional<bool> commandActive = std::nullopt);

        /**
         * @brief Sends a generic notification.
         * @param title The title of the notification.
         * @param body The body of the notification.
         * @param severity The severity of the notification.
         */
        void notify(const std::string& title, const std::string& body, NotificationSeverity severity = NotificationSeverity::Info);

        /**
         * @brief Adds a notification sink (primarily for testing).
         * @param sink A unique pointer to the notification sink.
         */
        void addSink(std::unique_ptr<INotificationSink> sink) { m_sinks.push_back(std::move(sink)); }

    private:
        StatusNotificationMode m_mode;
        bool m_jsonOutput;
        std::vector<std::unique_ptr<INotificationSink>> m_sinks;
    };
}

#endif // INCLUDE_AUTOINPUT_PLATFORM_NOTIFICATIONS_H
