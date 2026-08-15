/**
 * @file dispatchTestLinux.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include "autoinput/input/keyboard.h"
#include "autoinput/input/mouse.h"
#include "autoinput/platform/linux/internalDataLinux.h"

// Mock implementations of platform-specific functions
namespace autoinput
{
    bool isX11KeyDown(const X11KeyboardData& data) { return data.type == 1; }
    bool isWaylandKeyDown(const WaylandKeyboardData& data) { return data.value == 1; }
    bool isX11KeyUp(const X11KeyboardData& data) { return data.type == 2; }
    bool isWaylandKeyUp(const WaylandKeyboardData& data) { return data.value == 0; }
    int8_t getX11Char(const X11KeyboardData& data) { return (int8_t)data.keycode; }
    int8_t getWaylandChar(const WaylandKeyboardData& data) { return (int8_t)data.code; }
    int32_t getX11VirtualKey(const X11KeyboardData& data) { return (int32_t)data.keycode; }
    int32_t getWaylandVirtualKey(const WaylandKeyboardData& data) { return (int32_t)data.code; }
    int64_t getX11FunctionKey(const X11KeyboardData& data) { return (int64_t)data.keycode; }
    int64_t getWaylandFunctionKey(const WaylandKeyboardData& data) { return (int64_t)data.code; }
    void printX11KeyboardInfo(const X11KeyboardData&) {}
    void printWaylandKeyboardInfo(const WaylandKeyboardData&) {}

    bool isX11LeftButtonDown(const X11MouseData& data) { return data.button == 1 && data.state == 1; }
    bool isWaylandLeftButtonDown(const WaylandMouseData& data) { return data.code == 0x110 && data.value == 1; }
    bool isX11LeftButtonUp(const X11MouseData& data) { return data.button == 1 && data.state == 0; }
    bool isWaylandLeftButtonUp(const WaylandMouseData& data) { return data.code == 0x110 && data.value == 0; }
    bool isX11RightButtonDown(const X11MouseData& data) { return data.button == 3 && data.state == 1; }
    bool isWaylandRightButtonDown(const WaylandMouseData& data) { return data.code == 0x111 && data.value == 1; }
    bool isX11RightButtonUp(const X11MouseData& data) { return data.button == 3 && data.state == 0; }
    bool isWaylandRightButtonUp(const WaylandMouseData& data) { return data.code == 0x111 && data.value == 0; }
    bool isX11MiddleButtonDown(const X11MouseData& data) { return data.button == 2 && data.state == 1; }
    bool isWaylandMiddleButtonDown(const WaylandMouseData& data) { return data.code == 0x112 && data.value == 1; }
    bool isX11MiddleButtonUp(const X11MouseData& data) { return data.button == 2 && data.state == 0; }
    bool isWaylandMiddleButtonUp(const WaylandMouseData& data) { return data.code == 0x112 && data.value == 0; }
    bool isX11BackButtonDown(const X11MouseData& data) { return data.button == 8 && data.state == 1; }
    bool isWaylandBackButtonDown(const WaylandMouseData& data) { return data.code == 0x113 && data.value == 1; }
    bool isX11BackButtonUp(const X11MouseData& data) { return data.button == 8 && data.state == 0; }
    bool isWaylandBackButtonUp(const WaylandMouseData& data) { return data.code == 0x113 && data.value == 0; }
    bool isX11ForwardButtonDown(const X11MouseData& data) { return data.button == 9 && data.state == 1; }
    bool isWaylandForwardButtonDown(const WaylandMouseData& data) { return data.code == 0x114 && data.value == 1; }
    bool isX11ForwardButtonUp(const X11MouseData& data) { return data.button == 9 && data.state == 0; }
    bool isWaylandForwardButtonUp(const WaylandMouseData& data) { return data.code == 0x114 && data.value == 0; }
    void printX11MouseInfo(const X11MouseData&) {}
    void printWaylandMouseInfo(const WaylandMouseData&) {}

}

namespace linux_dispatch = autoinput::linux_dispatch;

TEST(LinuxDispatchTest, KeyboardX11)
{
    autoinput::KeyboardData data;
    autoinput::X11KeyboardData x11;
    x11.type = 1;
    x11.keycode = 65;
    data.internal = x11;

    EXPECT_TRUE(linux_dispatch::Keyboard_isKeyDown(data));
    EXPECT_FALSE(linux_dispatch::Keyboard_isKeyUp(data));
    EXPECT_EQ(linux_dispatch::Keyboard_getChar(data), 65);
}

TEST(LinuxDispatchTest, KeyboardWayland)
{
    autoinput::KeyboardData data;
    autoinput::WaylandKeyboardData way;
    way.value = 1;
    way.code = 66;
    data.internal = way;

    EXPECT_TRUE(linux_dispatch::Keyboard_isKeyDown(data));
    EXPECT_FALSE(linux_dispatch::Keyboard_isKeyUp(data));
    EXPECT_EQ(linux_dispatch::Keyboard_getChar(data), 66);
}

TEST(LinuxDispatchTest, MouseX11)
{
    autoinput::MouseData data;
    autoinput::X11MouseData x11;
    x11.button = 1;
    x11.state = 1;
    data.internal = x11;

    EXPECT_TRUE(linux_dispatch::Mouse_isLeftButtonDown(data));
    EXPECT_FALSE(linux_dispatch::Mouse_isLeftButtonUp(data));
}

TEST(LinuxDispatchTest, MouseWayland)
{
    autoinput::MouseData data;
    autoinput::WaylandMouseData way;
    way.code = 0x110;
    way.value = 1;
    data.internal = way;

    EXPECT_TRUE(linux_dispatch::Mouse_isLeftButtonDown(data));
    EXPECT_FALSE(linux_dispatch::Mouse_isLeftButtonUp(data));
}

TEST(LinuxDispatchTest, NoMatch)
{
    autoinput::KeyboardData data;
    // No internal data set
    EXPECT_FALSE(linux_dispatch::Keyboard_isKeyDown(data));
    
    autoinput::MouseData mdata;
    EXPECT_FALSE(linux_dispatch::Mouse_isLeftButtonDown(mdata));
}
