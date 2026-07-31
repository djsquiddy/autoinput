#include <gtest/gtest.h>
#include "autoinput/mouse.h"
#include "autoinput/win32/internalData_win32.h"

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
        EXPECT_TRUE(input.isBackButtonDown());
        EXPECT_EQ(input.getButtonState().button, MouseButton::BACK);
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
        EXPECT_TRUE(input.isForwardButtonDown());
        EXPECT_EQ(input.getButtonState().button, MouseButton::FORWARD);
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
        EXPECT_TRUE(input.isBackButtonUp());
        EXPECT_EQ(input.getButtonState().button, MouseButton::BACK);
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
        EXPECT_TRUE(input.isForwardButtonUp());
        EXPECT_EQ(input.getButtonState().button, MouseButton::FORWARD);
        EXPECT_FALSE(input.getButtonState().isDown);
    }
}
#endif
