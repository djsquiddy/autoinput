/**
 * @file internalData_linux.h
 * @author djsquiddy
 * @date July 2026
 */
#ifndef INCLUDE_AUTOINPUT_LINUX_INTERNALDATA_LINUX_H
#define INCLUDE_AUTOINPUT_LINUX_INTERNALDATA_LINUX_H
#pragma once

#include "autoinput/backend.h"

#if defined(__linux__)
#if AUTOINPUT_WITH_X11
#include <X11/Xlib.h> // IWYU pragma: export
#include <X11/Xutil.h> // IWYU pragma: export
#include <X11/keysym.h> // IWYU pragma: export
#include <X11/extensions/XTest.h> // IWYU pragma: export
#include <X11/Xos.h> // IWYU pragma: export
#include <X11/Xatom.h> // IWYU pragma: export
#endif
#include <linux/uinput.h> // IWYU pragma: export
#include <linux/input.h> // IWYU pragma: export
#include <fcntl.h> // IWYU pragma: export
#include <unistd.h> // IWYU pragma: export
#include <poll.h> // IWYU pragma: export
#endif

namespace autoinput
{
#if defined(__linux__) && AUTOINPUT_WITH_X11
    struct X11KeyboardData
    {
        XKeyEvent event;
    };

    struct X11MouseData
    {
        XButtonEvent event;
    };
#else
    // Fallback/Mock structures for testing on other platforms or when X11 is missing
    struct X11KeyboardData
    {
        int type;
        unsigned int keycode;
        unsigned int state;
        // Add other fields if needed for tests
    };

    struct X11MouseData
    {
        int type;
        unsigned int button;
        unsigned int state;
    };
#endif

    struct WaylandKeyboardData
    {
        uint16_t code;
        int value;
    };

    struct WaylandMouseData
    {
        uint16_t code;
        int value;
    };

    std::unique_ptr<PlatformBackend> createWaylandBackend();
    std::unique_ptr<PlatformBackend> createX11Backend();

    // Platform-specific input helper functions
    bool isX11KeyDown(const X11KeyboardData& data);
    bool isWaylandKeyDown(const WaylandKeyboardData& data);
    bool isX11KeyUp(const X11KeyboardData& data);
    bool isWaylandKeyUp(const WaylandKeyboardData& data);
    int8_t getX11Char(const X11KeyboardData& data);
    int8_t getWaylandChar(const WaylandKeyboardData& data);
    int32_t getX11VirtualKey(const X11KeyboardData& data);
    int32_t getWaylandVirtualKey(const WaylandKeyboardData& data);
    int64_t getX11FunctionKey(const X11KeyboardData& data);
    int64_t getWaylandFunctionKey(const WaylandKeyboardData& data);
    void printX11KeyboardInfo(const X11KeyboardData& data);
    void printWaylandKeyboardInfo(const WaylandKeyboardData& data);

    bool isX11LeftButtonDown(const X11MouseData& data);
    bool isWaylandLeftButtonDown(const WaylandMouseData& data);
    bool isX11LeftButtonUp(const X11MouseData& data);
    bool isWaylandLeftButtonUp(const WaylandMouseData& data);
    bool isX11RightButtonDown(const X11MouseData& data);
    bool isWaylandRightButtonDown(const WaylandMouseData& data);
    bool isX11RightButtonUp(const X11MouseData& data);
    bool isWaylandRightButtonUp(const WaylandMouseData& data);
    bool isX11MiddleButtonDown(const X11MouseData& data);
    bool isWaylandMiddleButtonDown(const WaylandMouseData& data);
    bool isX11MiddleButtonUp(const X11MouseData& data);
    bool isWaylandMiddleButtonUp(const WaylandMouseData& data);
    bool isX11BackButtonDown(const X11MouseData& data);
    bool isWaylandBackButtonDown(const WaylandMouseData& data);
    bool isX11BackButtonUp(const X11MouseData& data);
    bool isWaylandBackButtonUp(const WaylandMouseData& data);
    bool isX11ForwardButtonDown(const X11MouseData& data);
    bool isWaylandForwardButtonDown(const WaylandMouseData& data);
    bool isX11ForwardButtonUp(const X11MouseData& data);
    bool isWaylandForwardButtonUp(const WaylandMouseData& data);
    void printX11MouseInfo(const X11MouseData& data);
    void printWaylandMouseInfo(const WaylandMouseData& data);

    namespace linux_dispatch
    {
        bool Keyboard_isKeyDown(const KeyboardData& data);
        bool Keyboard_isKeyUp(const KeyboardData& data);
        int8_t Keyboard_getChar(const KeyboardData& data);
        int64_t Keyboard_functionKey(const KeyboardData& data);
        int32_t Keyboard_getVirtualKey(const KeyboardData& data);
        void Keyboard_printInfo(const KeyboardData& data);

        bool Mouse_isLeftButtonDown(const MouseData& data);
        bool Mouse_isLeftButtonUp(const MouseData& data);
        bool Mouse_isRightButtonDown(const MouseData& data);
        bool Mouse_isRightButtonUp(const MouseData& data);
        bool Mouse_isMiddleButtonDown(const MouseData& data);
        bool Mouse_isMiddleButtonUp(const MouseData& data);
        bool Mouse_isBackButtonDown(const MouseData& data);
        bool Mouse_isBackButtonUp(const MouseData& data);
        bool Mouse_isForwardButtonDown(const MouseData& data);
        bool Mouse_isForwardButtonUp(const MouseData& data);
        void Mouse_printInfo(const MouseData& data);

        std::string getActiveApplicationName();
        std::vector<std::string> getRunningApplicationNames();
    }
}

#endif // INCLUDE_AUTOINPUT_LINUX_INTERNALDATA_LINUX_H
