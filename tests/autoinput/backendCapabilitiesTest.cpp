/**
 * @file backendCapabilitiesTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include "autoinput/backend.h"

namespace autoinput
{
    TEST(BackendCapabilitiesTest, DefaultCapabilitiesAreFalse)
    {
        BackendCapabilities caps;
        EXPECT_FALSE(caps.keyboardHooks);
        EXPECT_FALSE(caps.mouseHooks);
        EXPECT_FALSE(caps.focusDetection);
        EXPECT_FALSE(caps.listApplications);
        EXPECT_FALSE(caps.syntheticKeyboardInput);
        EXPECT_FALSE(caps.syntheticMouseInput);
    }

    TEST(BackendCapabilitiesTest, FakeBackendHasAllCapabilities)
    {
        FakeBackend backend;
        BackendCapabilities caps = backend.capabilities();

        EXPECT_TRUE(caps.keyboardHooks);
        EXPECT_TRUE(caps.mouseHooks);
        EXPECT_TRUE(caps.focusDetection);
        EXPECT_TRUE(caps.listApplications);
        EXPECT_TRUE(caps.syntheticKeyboardInput);
        EXPECT_TRUE(caps.syntheticMouseInput);
    }
}
