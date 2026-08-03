/**
 * @file notifications.h
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_NOTIFICATIONS_H
#define INCLUDE_AUTOINPUT_NOTIFICATIONS_H
#pragma once

#include <string>
#include <memory>
#include <vector>
#include <optional>
#include "autoinput/types.h"

namespace autoinput
{
    class INotificationSink
    {
    public:
        virtual ~INotificationSink() = default;
        virtual void notify(const std::string& title, const std::string& body) = 0;
    };

    class NotificationService
    {
    public:
        NotificationService(StatusNotificationMode mode, bool jsonOutput);
        
        void notifyStatus(bool active, const std::string& commandName = "", std::optional<bool> commandActive = std::nullopt);

        // For testing
        void addSink(std::unique_ptr<INotificationSink> sink) { m_sinks.push_back(std::move(sink)); }

    private:
        StatusNotificationMode m_mode;
        bool m_jsonOutput;
        std::vector<std::unique_ptr<INotificationSink>> m_sinks;
    };
}

#endif // INCLUDE_AUTOINPUT_NOTIFICATIONS_H
