/**
 * @file notificationWin32Test.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include "autoinput/platform/notifications.h"
#include "autoinput/platform/platform.h"

#ifdef _WIN32
#include <windows.h>

namespace autoinput::testing
{
    TEST(WindowsNotificationSinkTest, CanConstructAndDestruct)
    {
        auto sink = platform::createDesktopNotificationSink();
        EXPECT_NE(sink, nullptr);
    }

    TEST(WindowsNotificationSinkTest, NotifyIsSafe)
    {
        auto sink = platform::createDesktopNotificationSink();
        ASSERT_NE(sink, nullptr);
        
        // This should not crash even if it fails to show the notification (e.g. no shell)
        sink->notify("Test Title", "Test Body");
        sink->notify("Test Title 2", "Test Body 2");
    }

    TEST(WindowsNotificationSinkTest, MultipleSinksAreSafe)
    {
        auto sink1 = platform::createDesktopNotificationSink();
        auto sink2 = platform::createDesktopNotificationSink();
        
        ASSERT_NE(sink1, nullptr);
        ASSERT_NE(sink2, nullptr);
        
        sink1->notify("Sink 1", "Message 1");
        sink2->notify("Sink 2", "Message 2");
    }
}
#endif
