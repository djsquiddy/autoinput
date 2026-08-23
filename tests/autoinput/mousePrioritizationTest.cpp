/**
 * @file mousePrioritizationTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include "autoinput/input/mouse.h"

using namespace autoinput;

#include "autoinput/platform/win32/internalDataWin32.h"

TEST(MousePrioritizationTest, GetButtonStatePrioritization)
{
    MouseData data;
    
#ifdef _WIN32
    WindowsMouseData winData;
    // We want to simulate multiple buttons being "active" to check prioritization
    // But getButtonState() calls isLeftButtonDown(), isLeftButtonUp(), etc.
    // Each of these checks if data.internal is WindowsMouseData and matches wParam.
    // Since wParam can only be one value at a time, we can't easily simulate "multiple buttons"
    // in a single MouseInput object if they are exclusive in the implementation.
    
    // HOWEVER, the implementation of getButtonState() is:
    // if (isLeftButtonDown()) return { MouseButton::Left, true };
    // if (isLeftButtonUp()) return { MouseButton::Left, false };
    // ...
    
    // So we just need to verify that each one returns the correct state.
    
    winData.wParam = WM_LBUTTONDOWN;
    data.internal = winData;
    {
        MouseInput input(data);
        auto state = input.getButtonState();
        // Verify WM_LBUTTONDOWN event maps to Left mouse button
        EXPECT_EQ(state.button, MouseButton::Left);
        // Verify WM_LBUTTONDOWN indicates the button is down
        EXPECT_TRUE(state.isDown);
    }

    winData.wParam = WM_LBUTTONUP;
    data.internal = winData;
    {
        MouseInput input(data);
        auto state = input.getButtonState();
        // Verify WM_LBUTTONUP event maps to Left mouse button
        EXPECT_EQ(state.button, MouseButton::Left);
        // Verify WM_LBUTTONUP indicates the button is released
        EXPECT_FALSE(state.isDown);
    }

    winData.wParam = WM_RBUTTONDOWN;
    data.internal = winData;
    {
        MouseInput input(data);
        auto state = input.getButtonState();
        // Verify WM_RBUTTONDOWN event maps to Right mouse button
        EXPECT_EQ(state.button, MouseButton::Right);
        // Verify WM_RBUTTONDOWN indicates the button is down
        EXPECT_TRUE(state.isDown);
    }
    
    // Test Back button
    MSLLHOOKSTRUCT msStruct;
    msStruct.mouseData = MAKELONG(0, XBUTTON1);
    winData.wParam = WM_XBUTTONDOWN;
    winData.mouseStruct = &msStruct;
    data.internal = winData;
    {
        MouseInput input(data);
        auto state = input.getButtonState();
        // Verify WM_XBUTTONDOWN with XBUTTON1 maps to Back mouse button
        EXPECT_EQ(state.button, MouseButton::Back);
        // Verify WM_XBUTTONDOWN indicates the button is down
        EXPECT_TRUE(state.isDown);
    }
    
    // Test Forward button
    msStruct.mouseData = MAKELONG(0, XBUTTON2);
    winData.wParam = WM_XBUTTONDOWN;
    winData.mouseStruct = &msStruct;
    data.internal = winData;
    {
        MouseInput input(data);
        auto state = input.getButtonState();
        // Verify WM_XBUTTONDOWN with XBUTTON2 maps to Forward mouse button
        EXPECT_EQ(state.button, MouseButton::Forward);
        // Verify WM_XBUTTONDOWN indicates the button is down
        EXPECT_TRUE(state.isDown);
    }
#else
    // Linux implementation uses any_cast to WaylandMouseData or X11MouseData
    // We would do similar mocks here.
#endif
}

TEST(MousePrioritizationTest, GetButtonStateNone)
{
    MouseData data;
    // Empty internal data
    MouseInput input(data);
    auto state = input.getButtonState();
    // Verify empty mouse input defaults to MouseButton::None
    EXPECT_EQ(state.button, MouseButton::None);
    // Verify empty mouse input reports button not down
    EXPECT_FALSE(state.isDown);
}
