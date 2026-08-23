/**
 * @file mouseTestWin32.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include "autoinput/input/mouse.h"
#include "autoinput/platform/win32/internalDataWin32.h"

#ifdef _WIN32
#include <windows.h>

namespace autoinput
{
    TEST(WindowsMouseInputTest, DetectsBackButtonDown)
    {
        MouseData data;
        MSLLHOOKSTRUCT mouseStruct{};
        mouseStruct.mouseData = MAKELONG(0, XBUTTON1); // XBUTTON1 in high word
        
        WindowsMouseData winData;
        winData.wParam = WM_XBUTTONDOWN;
        winData.mouseStruct = &mouseStruct;
        data.internal = winData;

        MouseInput input(data);
        // Verify isBackButtonDown detects XBUTTON1 down event
        EXPECT_TRUE(input.isBackButtonDown());
        // Verify getButtonState reports Back mouse button
        EXPECT_EQ(input.getButtonState().button, MouseButton::Back);
        // Verify getButtonState reports button is pressed down
        EXPECT_TRUE(input.getButtonState().isDown);
    }

    TEST(WindowsMouseInputTest, DetectsForwardButtonDown)
    {
        MouseData data;
        MSLLHOOKSTRUCT mouseStruct{};
        mouseStruct.mouseData = MAKELONG(0, XBUTTON2); // XBUTTON2 in high word
        
        WindowsMouseData winData;
        winData.wParam = WM_XBUTTONDOWN;
        winData.mouseStruct = &mouseStruct;
        data.internal = winData;

        MouseInput input(data);
        // Verify isForwardButtonDown detects XBUTTON2 down event
        EXPECT_TRUE(input.isForwardButtonDown());
        // Verify getButtonState reports Forward mouse button
        EXPECT_EQ(input.getButtonState().button, MouseButton::Forward);
        // Verify getButtonState reports button is pressed down
        EXPECT_TRUE(input.getButtonState().isDown);
    }

    TEST(WindowsMouseInputTest, DetectsBackButtonUp)
    {
        MouseData data;
        MSLLHOOKSTRUCT mouseStruct{};
        mouseStruct.mouseData = MAKELONG(0, XBUTTON1); // XBUTTON1 in high word
        
        WindowsMouseData winData;
        winData.wParam = WM_XBUTTONUP;
        winData.mouseStruct = &mouseStruct;
        data.internal = winData;

        MouseInput input(data);
        // Verify isBackButtonUp detects XBUTTON1 up event
        EXPECT_TRUE(input.isBackButtonUp());
        // Verify getButtonState reports Back mouse button
        EXPECT_EQ(input.getButtonState().button, MouseButton::Back);
        // Verify getButtonState reports button is released
        EXPECT_FALSE(input.getButtonState().isDown);
    }

    TEST(WindowsMouseInputTest, DetectsForwardButtonUp)
    {
        MouseData data;
        MSLLHOOKSTRUCT mouseStruct{};
        mouseStruct.mouseData = MAKELONG(0, XBUTTON2); // XBUTTON2 in high word
        
        WindowsMouseData winData;
        winData.wParam = WM_XBUTTONUP;
        winData.mouseStruct = &mouseStruct;
        data.internal = winData;

        MouseInput input(data);
        // Verify isForwardButtonUp detects XBUTTON2 up event
        EXPECT_TRUE(input.isForwardButtonUp());
        // Verify getButtonState reports Forward mouse button
        EXPECT_EQ(input.getButtonState().button, MouseButton::Forward);
        // Verify getButtonState reports button is released
        EXPECT_FALSE(input.getButtonState().isDown);
    }
}
#endif
