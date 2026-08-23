/**
 * @file notificationTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "autoinput/platform/notifications.h"
#include "autoinput/support/types.h"

namespace autoinput::testing
{
    class MockNotificationSink : public INotificationSink
    {
    public:
        MOCK_METHOD(void, notify, (const std::string& title, const std::string& body, NotificationSeverity severity), (override));
    };

    TEST(NotificationTest, OffSendsNothing)
    {
        NotificationService service(StatusNotificationMode::Off, false);
        auto mockSink = std::make_unique<MockNotificationSink>();
        // Verify that no notification is sent when notification mode is set to Off
        EXPECT_CALL(*mockSink, notify(::testing::_, ::testing::_, ::testing::_)).Times(0);
        
        service.addSink(std::move(mockSink));
        service.notifyStatus(true);
    }

    TEST(NotificationTest, JsonSuppressesNotifications)
    {
        NotificationService service(StatusNotificationMode::Both, true);
        auto mockSink = std::make_unique<MockNotificationSink>();
        // Verify that no notification is sent when JSON mode suppresses notifications
        EXPECT_CALL(*mockSink, notify(::testing::_, ::testing::_, ::testing::_)).Times(0);
        
        service.addSink(std::move(mockSink));
        service.notifyStatus(true);
    }

    TEST(NotificationTest, NotifyActive)
    {
        NotificationService service(StatusNotificationMode::Desktop, false);
        auto mockSink = std::make_unique<MockNotificationSink>();
        // Verify that active status sends notification with title, active message, and Info severity
        EXPECT_CALL(*mockSink, notify("AutoInput", "Auto clicking: ACTIVE", NotificationSeverity::Info)).Times(1);
        
        service.addSink(std::move(mockSink));
        service.notifyStatus(true);
    }

    TEST(NotificationTest, NotifyPaused)
    {
        NotificationService service(StatusNotificationMode::Desktop, false);
        auto mockSink = std::make_unique<MockNotificationSink>();
        // Verify that paused status sends notification with title, paused message, and Warning severity
        EXPECT_CALL(*mockSink, notify("AutoInput", "Auto clicking: PAUSED", NotificationSeverity::Warning)).Times(1);
        
        service.addSink(std::move(mockSink));
        service.notifyStatus(false);
    }

    TEST(NotificationTest, MultipleSinks)
    {
        NotificationService service(StatusNotificationMode::Both, false);
        auto mockSink1 = std::make_unique<MockNotificationSink>();
        auto mockSink2 = std::make_unique<MockNotificationSink>();
        
        // Verify that all registered notification sinks receive the status notification
        EXPECT_CALL(*mockSink1, notify(::testing::_, ::testing::_, ::testing::_)).Times(1);
        EXPECT_CALL(*mockSink2, notify(::testing::_, ::testing::_, ::testing::_)).Times(1);
        
        service.addSink(std::move(mockSink1));
        service.addSink(std::move(mockSink2));
        service.notifyStatus(true);
    }
}
