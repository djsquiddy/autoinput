/**
 * @file autoInput_linux.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include "autoinput/autoInput.h"
#include "autoinput/mouse.h"
#include "autoinput/keyboard.h"
#include "autoinput/logger.h"
#include "autoinput/types.h"
#include "autoinput/platform.h"
#include "autoinput/backend.h"
#include "autoinput/linux/internalData_linux.h"

namespace autoinput
{
    namespace linux_dispatch
    {
        bool Keyboard_isKeyDown(const KeyboardData& data)
        {
            if (const auto* x11 = std::any_cast<X11KeyboardData>(&data.internal)) return isX11KeyDown(*x11);
            if (const auto* way = std::any_cast<WaylandKeyboardData>(&data.internal)) return isWaylandKeyDown(*way);
            return false;
        }

        bool Keyboard_isKeyUp(const KeyboardData& data)
        {
            if (const auto* x11 = std::any_cast<X11KeyboardData>(&data.internal)) return isX11KeyUp(*x11);
            if (const auto* way = std::any_cast<WaylandKeyboardData>(&data.internal)) return isWaylandKeyUp(*way);
            return false;
        }

        int8_t Keyboard_getChar(const KeyboardData& data)
        {
            if (const auto* x11 = std::any_cast<X11KeyboardData>(&data.internal)) return getX11Char(*x11);
            if (const auto* way = std::any_cast<WaylandKeyboardData>(&data.internal)) return getWaylandChar(*way);
            return INVALID_KEY;
        }

        int64_t Keyboard_functionKey(const KeyboardData& data)
        {
            if (const auto* x11 = std::any_cast<X11KeyboardData>(&data.internal)) return getX11FunctionKey(*x11);
            if (const auto* way = std::any_cast<WaylandKeyboardData>(&data.internal)) return getWaylandFunctionKey(*way);
            return INVALID_KEY;
        }

        int32_t Keyboard_getVirtualKey(const KeyboardData& data)
        {
            if (const auto* x11 = std::any_cast<X11KeyboardData>(&data.internal)) return getX11VirtualKey(*x11);
            if (const auto* way = std::any_cast<WaylandKeyboardData>(&data.internal)) return getWaylandVirtualKey(*way);
            return 0;
        }

        void Keyboard_printInfo(const KeyboardData& data)
        {
            if (const auto* x11 = std::any_cast<X11KeyboardData>(&data.internal)) printX11KeyboardInfo(*x11);
            if (const auto* way = std::any_cast<WaylandKeyboardData>(&data.internal)) printWaylandKeyboardInfo(*way);
        }

        bool Mouse_isLeftButtonDown(const MouseData& data)
        {
            if (const auto* x11 = std::any_cast<X11MouseData>(&data.internal)) return isX11LeftButtonDown(*x11);
            if (const auto* way = std::any_cast<WaylandMouseData>(&data.internal)) return isWaylandLeftButtonDown(*way);
            return false;
        }

        bool Mouse_isLeftButtonUp(const MouseData& data)
        {
            if (const auto* x11 = std::any_cast<X11MouseData>(&data.internal)) return isX11LeftButtonUp(*x11);
            if (const auto* way = std::any_cast<WaylandMouseData>(&data.internal)) return isWaylandLeftButtonUp(*way);
            return false;
        }

        bool Mouse_isRightButtonDown(const MouseData& data)
        {
            if (const auto* x11 = std::any_cast<X11MouseData>(&data.internal)) return isX11RightButtonDown(*x11);
            if (const auto* way = std::any_cast<WaylandMouseData>(&data.internal)) return isWaylandRightButtonDown(*way);
            return false;
        }

        bool Mouse_isRightButtonUp(const MouseData& data)
        {
            if (const auto* x11 = std::any_cast<X11MouseData>(&data.internal)) return isX11RightButtonUp(*x11);
            if (const auto* way = std::any_cast<WaylandMouseData>(&data.internal)) return isWaylandRightButtonUp(*way);
            return false;
        }

        bool Mouse_isMiddleButtonDown(const MouseData& data)
        {
            if (const auto* x11 = std::any_cast<X11MouseData>(&data.internal)) return isX11MiddleButtonDown(*x11);
            if (const auto* way = std::any_cast<WaylandMouseData>(&data.internal)) return isWaylandMiddleButtonDown(*way);
            return false;
        }

        bool Mouse_isMiddleButtonUp(const MouseData& data)
        {
            if (const auto* x11 = std::any_cast<X11MouseData>(&data.internal)) return isX11MiddleButtonUp(*x11);
            if (const auto* way = std::any_cast<WaylandMouseData>(&data.internal)) return isWaylandMiddleButtonUp(*way);
            return false;
        }

        bool Mouse_isBackButtonDown(const MouseData& data)
        {
            if (const auto* x11 = std::any_cast<X11MouseData>(&data.internal)) return isX11BackButtonDown(*x11);
            if (const auto* way = std::any_cast<WaylandMouseData>(&data.internal)) return isWaylandBackButtonDown(*way);
            return false;
        }

        bool Mouse_isBackButtonUp(const MouseData& data)
        {
            if (const auto* x11 = std::any_cast<X11MouseData>(&data.internal)) return isX11BackButtonUp(*x11);
            if (const auto* way = std::any_cast<WaylandMouseData>(&data.internal)) return isWaylandBackButtonUp(*way);
            return false;
        }

        bool Mouse_isForwardButtonDown(const MouseData& data)
        {
            if (const auto* x11 = std::any_cast<X11MouseData>(&data.internal)) return isX11ForwardButtonDown(*x11);
            if (const auto* way = std::any_cast<WaylandMouseData>(&data.internal)) return isWaylandForwardButtonDown(*way);
            return false;
        }

        bool Mouse_isForwardButtonUp(const MouseData& data)
        {
            if (const auto* x11 = std::any_cast<X11MouseData>(&data.internal)) return isX11ForwardButtonUp(*x11);
            if (const auto* way = std::any_cast<WaylandMouseData>(&data.internal)) return isWaylandForwardButtonUp(*way);
            return false;
        }

        void Mouse_printInfo(const MouseData& data)
        {
            if (const auto* x11 = std::any_cast<X11MouseData>(&data.internal)) printX11MouseInfo(*x11);
            if (const auto* way = std::any_cast<WaylandMouseData>(&data.internal)) printWaylandMouseInfo(*way);
        }

        std::string getActiveApplicationName()
        {
            extern std::string getX11ActiveApplicationName();
            extern std::string getWaylandActiveApplicationName();

            const char* sessionType = std::getenv("XDG_SESSION_TYPE");
            if (sessionType && std::string(sessionType) == "wayland")
            {
                return getWaylandActiveApplicationName();
            }
            return getX11ActiveApplicationName();
        }

        std::vector<std::string> getRunningApplicationNames()
        {
            extern std::vector<std::string> getX11RunningApplicationNames();
            extern std::vector<std::string> getWaylandRunningApplicationNames();

            const char* sessionType = std::getenv("XDG_SESSION_TYPE");
            if (sessionType && std::string(sessionType) == "wayland")
            {
                return getWaylandRunningApplicationNames();
            }
            return getX11RunningApplicationNames();
        }
    }

#if defined(__linux__)
    KeyboardInput::KeyboardInput(KeyboardData& data) : data{ data } {}
    
    bool KeyboardInput::isKeyDown() const
    {
        return linux_dispatch::Keyboard_isKeyDown(data);
    }

    bool KeyboardInput::isKeyUp() const
    {
        return linux_dispatch::Keyboard_isKeyUp(data);
    }

    bool KeyboardInput::isSysKey() const { return false; }

    int8_t KeyboardInput::getChar() const
    {
        return linux_dispatch::Keyboard_getChar(data);
    }

    int64_t KeyboardInput::functionKey() const
    {
        return linux_dispatch::Keyboard_functionKey(data);
    }

    KeyboardInput::KeyState KeyboardInput::getKeyState() const
    {
        return { 
            static_cast<int32_t>(getChar()), 
            static_cast<int32_t>(functionKey()), 
            linux_dispatch::Keyboard_getVirtualKey(data), 
            KeyModifier::None 
        };
    }

    void KeyboardInput::printInfo() const
    {
        linux_dispatch::Keyboard_printInfo(data);
    }

    MouseInput::MouseInput(MouseData& data) : data{ data } {}
    
    bool MouseInput::isLeftButtonDown() const
    {
        return linux_dispatch::Mouse_isLeftButtonDown(data);
    }

    bool MouseInput::isLeftButtonUp() const
    {
        return linux_dispatch::Mouse_isLeftButtonUp(data);
    }

    bool MouseInput::isRightButtonDown() const
    {
        return linux_dispatch::Mouse_isRightButtonDown(data);
    }

    bool MouseInput::isRightButtonUp() const
    {
        return linux_dispatch::Mouse_isRightButtonUp(data);
    }

    bool MouseInput::isMiddleButtonDown() const
    {
        return linux_dispatch::Mouse_isMiddleButtonDown(data);
    }

    bool MouseInput::isMiddleButtonUp() const
    {
        return linux_dispatch::Mouse_isMiddleButtonUp(data);
    }

    bool MouseInput::isBackButtonDown() const
    {
        return linux_dispatch::Mouse_isBackButtonDown(data);
    }

    bool MouseInput::isBackButtonUp() const
    {
        return linux_dispatch::Mouse_isBackButtonUp(data);
    }

    bool MouseInput::isForwardButtonDown() const
    {
        return linux_dispatch::Mouse_isForwardButtonDown(data);
    }

    bool MouseInput::isForwardButtonUp() const
    {
        return linux_dispatch::Mouse_isForwardButtonUp(data);
    }

    MouseInput::ButtonState MouseInput::getButtonState() const
    {
        if (isLeftButtonDown()) return { MouseButton::LEFT, true };
        if (isLeftButtonUp()) return { MouseButton::LEFT, false };
        if (isRightButtonDown()) return { MouseButton::RIGHT, true };
        if (isRightButtonUp()) return { MouseButton::RIGHT, false };
        if (isMiddleButtonDown()) return { MouseButton::MIDDLE, true };
        if (isMiddleButtonUp()) return { MouseButton::MIDDLE, false };
        if (isBackButtonDown()) return { MouseButton::BACK, true };
        if (isBackButtonUp()) return { MouseButton::BACK, false };
        if (isForwardButtonDown()) return { MouseButton::FORWARD, true };
        if (isForwardButtonUp()) return { MouseButton::FORWARD, false };
        return { MouseButton::NONE, false };
    }

    void MouseInput::printInfo() const
    {
        linux_dispatch::Mouse_printInfo(data);
    }
#endif


#if defined(__linux__)
    namespace platform
    {
        void signalEnd()
        {
            if (g_backend) g_backend->cleanup();
#ifndef AUTOINPUT_TESTING
            std::exit(0);
#endif
        }

        namespace
        {
            void SignalHandler(int signum)
            {
                signalEnd();
            }
        }

        void setupSignalHandler()
        {
            std::signal(SIGINT, SignalHandler);
            std::signal(SIGTERM, SignalHandler);
        }

        int32_t getVirtualKey(const Key& key)
        {
            static const std::unordered_map<std::string, int32_t> linuxKeys = {
                {"esc", KEY_ESC},
                {"escape", KEY_ESC},
                {"space", KEY_SPACE},
                {"tab", KEY_TAB},
                {"enter", KEY_ENTER},
                {"return", KEY_ENTER},
                {"backspace", KEY_BACKSPACE},
                {"ins", KEY_INSERT},
                {"insert", KEY_INSERT},
                {"del", KEY_DELETE},
                {"delete", KEY_DELETE},
                {"home", KEY_HOME},
                {"end", KEY_END},
                {"pageup", KEY_PAGEUP},
                {"pgup", KEY_PAGEUP},
                {"pagedown", KEY_PAGEDOWN},
                {"pgdn", KEY_PAGEDOWN},
                {"up", KEY_UP},
                {"down", KEY_DOWN},
                {"left", KEY_LEFT},
                {"right", KEY_RIGHT},
                {"capslock", KEY_CAPSLOCK},
                {"numlock", KEY_NUMLOCK},
                {"scrolllock", KEY_SCROLLLOCK},
                {"printscreen", KEY_SYSRQ},
                {"prtsc", KEY_SYSRQ},
                {"pause", KEY_PAUSE}
            };

            auto it = linuxKeys.find(key.character);
            if (it != linuxKeys.end()) return it->second;
            return 0;
        }

        std::string getActiveApplicationName()
        {
            return linux_dispatch::getActiveApplicationName();
        }

        std::vector<std::string> getRunningApplicationNames()
        {
            return linux_dispatch::getRunningApplicationNames();
        }

        std::filesystem::path getExecutablePath()
        {
            char buffer[1024];
            ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
            if (len != -1)
            {
                buffer[len] = '\0';
                return std::filesystem::path(buffer).parent_path();
            }
            return std::filesystem::current_path();
        }

        std::filesystem::path getUserHomePath()
        {
            const char* home = std::getenv("HOME");
            if (home)
            {
                return std::filesystem::path(home);
            }
            return std::filesystem::path();
        }
    }
#endif

#if defined(__linux__)
    std::unique_ptr<IPlatformBackend> detectLinuxBackend()
#else
    std::unique_ptr<IPlatformBackend> detectLinuxBackend_unused()
#endif
    {
        const char* sessionType = std::getenv("XDG_SESSION_TYPE");
        bool isWayland = sessionType && std::string(sessionType) == "wayland";

        if (isWayland)
        {
            Logger::info("Detected Wayland session, using Wayland/uinput backend\n");
            return createWaylandBackend();
        }
        else
        {
#if AUTOINPUT_WITH_X11
            Logger::info("Detected X11 session or unknown, using X11 backend\n");
            return createX11Backend();
#else
            Logger::error("X11 support was not compiled in and this is not a Wayland session.\n");
            return nullptr;
#endif
        }
    }
}
