/**
 * @file startKeyToggleTest.cpp
 * @author djsquiddy
 * @date July 2026
*/
#include "testUtils.h"
#include "autoinput/autoinput.h"
#include "autoinput/backend.h"
#include <set>
#include <thread>
#include <gtest/gtest.h>

namespace autoinput
{
    class TrackingBackend : public FakeBackend
    {
    public:
        void mousePress(const Mouse& mouse) override { lastButton = mouse.button; pressCount++; pressedButtons.insert(mouse.button); }
        void mouseRelease(const Mouse& mouse) override { lastButton = mouse.button; releaseCount++; pressedButtons.erase(mouse.button); }
        
        MouseButton lastButton = MouseButton::None;
        int pressCount = 0;
        int releaseCount = 0;
        std::set<MouseButton> pressedButtons;
    };

    class StartKeyToggleTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
        }

        void TearDown() override
        {
        }
    };

    TEST_F(StartKeyToggleTest, StartKeyTogglesSingleAction)
    {
        Program program;
        char* argv[] = {(char*)"autoinput", (char*)"hold", (char*)"left", (char*)"-s", (char*)"f2"};
        int argc = sizeof(argv) / sizeof(char*);

        ASSERT_TRUE(program.arguments().parseArguments(gsl::make_span(argv, argc)));
        
        auto trackingBackend = std::make_unique<TrackingBackend>();
        auto* trackingBackendPtr = trackingBackend.get();
        program.setBackend(std::move(trackingBackend));
        ASSERT_TRUE(program.init());

        const auto& keyInfo = program.getKeyInfo();
        
        // Simulate F2 press (Start action)
        program.start(keyInfo[0]);
        EXPECT_EQ(trackingBackendPtr->pressCount, 1);
        EXPECT_EQ(trackingBackendPtr->releaseCount, 0);
        EXPECT_EQ(trackingBackendPtr->lastButton, MouseButton::Left);

        // Simulate F2 press again (Toggle action)
        program.start(keyInfo[0]);
        EXPECT_EQ(trackingBackendPtr->pressCount, 1);
        EXPECT_EQ(trackingBackendPtr->releaseCount, 1);
        EXPECT_EQ(trackingBackendPtr->lastButton, MouseButton::Left);
    }

    TEST_F(StartKeyToggleTest, DuplicateActionsCauseToggleIssue)
    {
        Program program;
        // Duplicate 'left' with same start key 'f2'
        char* argv[] = {(char*)"autoinput", (char*)"hold", (char*)"left", (char*)"left", (char*)"-s", (char*)"f2"};
        int argc = sizeof(argv) / sizeof(char*);

        ASSERT_TRUE(program.arguments().parseArguments(gsl::make_span(argv, argc)));
        
        auto trackingBackend = std::make_unique<TrackingBackend>();
        auto* trackingBackendPtr = trackingBackend.get();
        program.setBackend(std::move(trackingBackend));
        ASSERT_TRUE(program.init());

        const auto& keyInfo = program.getKeyInfo();
        // Expecting 2 KeyInfo for F2 (both for LEFT)
        ASSERT_GE(keyInfo.size(), 2);

        // Simulate F2 press event as it happens in processKeyEvent
        // We'll manually call start for all matches
        for (const auto& info : keyInfo)
        {
            if (info.isStartKey) // Simplified match for F2
            {
                program.start(info);
            }
        }
        
        // Expected: pressCount should be 1 (first start) and releaseCount should be 1 (second start toggles it off)
        // If this is the case, then pressing F2 once actually does nothing visible!
        EXPECT_EQ(trackingBackendPtr->pressCount, 1);
        EXPECT_EQ(trackingBackendPtr->releaseCount, 1);
    }
}
