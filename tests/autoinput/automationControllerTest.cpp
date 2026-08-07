/**
 * @file automationControllerTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include "autoinput/automationController.h"
#include "autoinput/arguments.h"
#include <chrono>
#include <thread>

namespace autoinput
{
    TEST(AutomationControllerTest, StartsAndStops)
    {
        AutomationController controller;
        ProgramArguments args;
        // Basic config that does nothing but start the listener
        args.endKey = "f10"; 

        EXPECT_FALSE(controller.running());
        
        // This might fail if hooks cannot be installed in the test environment
        // but it should at least not crash.
        bool started = controller.start(std::move(args));
        
        if (started) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            EXPECT_TRUE(controller.running());
            controller.stop();
            EXPECT_FALSE(controller.running());
        }
    }

    TEST(AutomationControllerTest, StatusCallbackFires)
    {
        AutomationController controller;
        ProgramArguments args;
        args.endKey = "f10";

        ProgramStatus lastStatus;
        bool callbackCalled = false;
        controller.setStatusCallback([&](const ProgramStatus& status) {
            lastStatus = status;
            callbackCalled = true;
        });

        if (controller.start(std::move(args))) {
            // Manually trigger a status update via the global pointer
            if (g_program) {
                g_program->updateStatusIndicator("test_cmd", true);
            }
            
            EXPECT_TRUE(callbackCalled);
            EXPECT_EQ(lastStatus.triggeredCommandName, "test_cmd");
            EXPECT_TRUE(lastStatus.triggeredCommandActive.value_or(false));
            
            controller.stop();
        }
        EXPECT_EQ(g_program, nullptr);
    }
    TEST(AutomationControllerTest, FailsWithInvalidConfig)
    {
        AutomationController controller;
        ProgramArguments args;
        // An invalid config that should fail init or hooks
        args.endKey = "invalid_key_name_that_does_not_exist";

        EXPECT_FALSE(controller.start(std::move(args)));
        EXPECT_FALSE(controller.running());
        EXPECT_EQ(g_program, nullptr);
    }
}
