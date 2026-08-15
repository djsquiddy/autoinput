/**
 * @file autoInputX11.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include "autoinput/app/autoinput.h"
#include "autoinput/input/mouse.h"
#include "autoinput/input/keyboard.h"
#include "autoinput/support/logger.h"
#include "autoinput/support/types.h"
#include "autoinput/platform/linux/internalDataLinux.h"
#include <format>
#include <filesystem>
#include <set>

namespace autoinput
{
    namespace
    {
        Display* g_display = nullptr;
        Window g_rootWindow = 0;
        Atom g_activeWindowAtom = 0;
        bool g_running = true;

        std::string getX11ActiveApplicationName();

        unsigned int toX11Modifier(KeyModifier modifier)
        {
            unsigned int x11Modifier = 0;
            if (static_cast<bool>(modifier & KeyModifier::Shift)) x11Modifier |= ShiftMask;
            if (static_cast<bool>(modifier & KeyModifier::Ctrl)) x11Modifier |= ControlMask;
            if (static_cast<bool>(modifier & KeyModifier::Alt)) x11Modifier |= Mod1Mask;
            if (static_cast<bool>(modifier & KeyModifier::Meta)) x11Modifier |= Mod4Mask;
            return x11Modifier;
        }

        KeySym toKeySym(const Key& key)
        {
            if (static_cast<bool>(key.modifier & KeyModifier::Function))
            {
                const int functionNumber = std::stoi(key.character);
                if (functionNumber >= 1 && functionNumber <= 24)
                {
                    return XK_F1 + functionNumber - 1;
                }
            }

            if (key.character.length() == 1)
            {
                const KeySym ks = XStringToKeysym(key.character.c_str());
                if (ks != NoSymbol) return ks;
                
                // Fallback for some characters
                return static_cast<KeySym>(key.character[0]);
            }

            return NoSymbol;
        }

        unsigned int toX11Button(MouseButton button)
        {
            switch (button)
            {
            case MouseButton::Left: return 1;
            case MouseButton::Middle: return 2;
            case MouseButton::Right: return 3;
            case MouseButton::Back: return 8;
            case MouseButton::Forward: return 9;
            default: return 0;
            }
        }

        AppWindowInfo getX11AppWindowInfo(Window window)
        {
            AppWindowInfo info;
            info.backendId = std::format("{}", window);

            // Window Title
            char* name = nullptr;
            if (XFetchName(g_display, window, &name))
            {
                if (name)
                {
                    info.windowTitle = name;
                    XFree(name);
                }
            }

            // If XFetchName fails, try _NET_WM_NAME
            if (info.windowTitle.empty())
            {
                Atom actual_type;
                int actual_format;
                unsigned long nitems, bytes_after;
                unsigned char* prop = nullptr;
                Atom netWmName = XInternAtom(g_display, "_NET_WM_NAME", False);
                Atom utf8String = XInternAtom(g_display, "UTF8_STRING", False);
                
                if (XGetWindowProperty(g_display, window, netWmName,
                    0, 1024, False, utf8String,
                    &actual_type, &actual_format, &nitems, &bytes_after, &prop) == Success)
                {
                    if (prop)
                    {
                        info.windowTitle = reinterpret_cast<char*>(prop);
                        XFree(prop);
                    }
                }
            }

            // PID
            Atom pidAtom = XInternAtom(g_display, "_NET_WM_PID", True);
            if (pidAtom != None)
            {
                Atom actual_type;
                int actual_format;
                unsigned long nitems, bytes_after;
                unsigned char* prop = nullptr;
                if (XGetWindowProperty(g_display, window, pidAtom, 0, 1, False, XA_CARDINAL,
                    &actual_type, &actual_format, &nitems, &bytes_after, &prop) == Success)
                {
                    if (prop)
                    {
                        info.pid = *reinterpret_cast<unsigned long*>(prop);
                        XFree(prop);
                    }
                }
            }

            // Process Name and Executable Path from PID
            if (info.pid > 0)
            {
                char buf[1024];
                std::string path = std::format("/proc/{}/exe", info.pid);
                ssize_t len = readlink(path.c_str(), buf, sizeof(buf) - 1);
                if (len != -1)
                {
                    buf[len] = '\0';
                    info.executablePath = buf;
                    info.processName = std::filesystem::path(buf).filename().string();
                }
            }
            
            // Fallback for process name if we couldn't get it from PID
            if (info.processName.empty())
            {
                 XClassHint classHint;
                 if (XGetClassHint(g_display, window, &classHint))
                 {
                     info.processName = classHint.res_name ? classHint.res_name : "";
                     if (classHint.res_name) XFree(classHint.res_name);
                     if (classHint.res_class) XFree(classHint.res_class);
                 }
            }

            return info;
        }

        class X11Backend : public IPlatformBackend
        {
        public:
            void requestStop() override
            {
                // TODO: Implement
            }

            bool installHooks() override
            {
                XInitThreads();
                g_display = XOpenDisplay(nullptr);
                if (!g_display)
                {
                    Logger::error("Failed to open X display\n");
                    return false;
                }

                g_rootWindow = DefaultRootWindow(g_display);

                for (const auto& info : g_program->getKeyInfo())
                {
                    if (info.triggerButton != MouseButton::None)
                    {
                        const unsigned int button = toX11Button(info.triggerButton);
                        XGrabButton(g_display, button, AnyModifier, g_rootWindow, False, 
                            ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);
                    }
                    else if (info.virtualKey != 0)
                    {
                        const KeyCode code = static_cast<KeyCode>(info.virtualKey + 8);
                        XGrabKey(g_display, code, AnyModifier, g_rootWindow, False, GrabModeAsync, GrabModeAsync);
                    }
                    else if (info.keyCode != INVALID_KEY)
                    {
                        const KeyCode code = XKeysymToKeycode(g_display, static_cast<KeySym>(info.keyCode));
                        XGrabKey(g_display, code, AnyModifier, g_rootWindow, False, GrabModeAsync, GrabModeAsync);
                    }
                    else if (info.functionKey != INVALID_KEY)
                    {
                        const KeyCode code = XKeysymToKeycode(g_display, XK_F1 + info.functionKey - 1);
                        XGrabKey(g_display, code, AnyModifier, g_rootWindow, False, GrabModeAsync, GrabModeAsync);
                    }
                }

                XSelectInput(g_display, g_rootWindow, PropertyChangeMask);
                g_activeWindowAtom = XInternAtom(g_display, "_NET_ACTIVE_WINDOW", False);

                if (g_program)
                {
                    g_program->onFocusChanged(getX11ActiveApplicationName());
                }

                return true;
            }

            void runListener() override
            {
                XEvent event;
                X11KeyboardData kbdData;
                X11MouseData mouseData;

                while (g_running)
                {
                    if (XPending(g_display))
                    {
                        XNextEvent(g_display, &event);
                        if (event.type == KeyPress || event.type == KeyRelease)
                        {
                            kbdData.event = event.xkey;
                            g_program->processKeyEvent(KeyboardInput{ KeyboardData{ kbdData } });
                        }
                        else if (event.type == ButtonPress || event.type == ButtonRelease)
                        {
                            mouseData.event = event.xbutton;
                            g_program->processMouseEvent(MouseInput{ MouseData{ mouseData } });
                        }
                        else if (event.type == PropertyNotify && event.xproperty.atom == g_activeWindowAtom)
                        {
                            g_program->onFocusChanged(getX11ActiveApplicationName());
                        }
                    }
                    else
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }
                }
            }

            void cleanup() override
            {
                if (g_display)
                {
                    XUngrabKey(g_display, AnyKey, AnyModifier, g_rootWindow);
                    XUngrabButton(g_display, AnyButton, AnyModifier, g_rootWindow);
                    XCloseDisplay(g_display);
                    g_display = nullptr;
                }
            }

            void requestStop() override
            {
                g_running = false;
            }

            void keyPress(const Key& key) override
            {
                if (!g_display) return;
                const KeySym ks = toKeySym(key);
                if (ks == NoSymbol) return;
                const KeyCode code = XKeysymToKeycode(g_display, ks);
                if (code == 0) return;

                // Modifiers
                if (static_cast<bool>(key.modifier & KeyModifier::Shift)) XTestFakeKeyEvent(g_display, XKeysymToKeycode(g_display, XK_Shift_L), True, 0);
                if (static_cast<bool>(key.modifier & KeyModifier::Ctrl)) XTestFakeKeyEvent(g_display, XKeysymToKeycode(g_display, XK_Control_L), True, 0);
                if (static_cast<bool>(key.modifier & KeyModifier::Alt)) XTestFakeKeyEvent(g_display, XKeysymToKeycode(g_display, XK_Alt_L), True, 0);
                if (static_cast<bool>(key.modifier & KeyModifier::Meta)) XTestFakeKeyEvent(g_display, XKeysymToKeycode(g_display, XK_Super_L), True, 0);

                XTestFakeKeyEvent(g_display, code, True, 0);
                XFlush(g_display);
            }

            void keyRelease(const Key& key) override
            {
                if (!g_display) return;
                const KeySym ks = toKeySym(key);
                if (ks == NoSymbol) return;
                const KeyCode code = XKeysymToKeycode(g_display, ks);
                if (code == 0) return;

                XTestFakeKeyEvent(g_display, code, False, 0);

                // Modifiers
                if (static_cast<bool>(key.modifier & KeyModifier::Meta)) XTestFakeKeyEvent(g_display, XKeysymToKeycode(g_display, XK_Super_L), False, 0);
                if (static_cast<bool>(key.modifier & KeyModifier::Alt)) XTestFakeKeyEvent(g_display, XKeysymToKeycode(g_display, XK_Alt_L), False, 0);
                if (static_cast<bool>(key.modifier & KeyModifier::Ctrl)) XTestFakeKeyEvent(g_display, XKeysymToKeycode(g_display, XK_Control_L), False, 0);
                if (static_cast<bool>(key.modifier & KeyModifier::Shift)) XTestFakeKeyEvent(g_display, XKeysymToKeycode(g_display, XK_Shift_L), False, 0);

                XFlush(g_display);
            }

            void mousePress(const Mouse& mouse) override
            {
                if (!g_display) return;
                const unsigned int xButton = toX11Button(mouse.button);
                if (xButton == 0) return;

                // Modifiers
                if (static_cast<bool>(mouse.modifier & KeyModifier::Shift)) XTestFakeKeyEvent(g_display, XKeysymToKeycode(g_display, XK_Shift_L), True, 0);
                if (static_cast<bool>(mouse.modifier & KeyModifier::Ctrl)) XTestFakeKeyEvent(g_display, XKeysymToKeycode(g_display, XK_Control_L), True, 0);
                if (static_cast<bool>(mouse.modifier & KeyModifier::Alt)) XTestFakeKeyEvent(g_display, XKeysymToKeycode(g_display, XK_Alt_L), True, 0);
                if (static_cast<bool>(mouse.modifier & KeyModifier::Meta)) XTestFakeKeyEvent(g_display, XKeysymToKeycode(g_display, XK_Super_L), True, 0);

                XTestFakeButtonEvent(g_display, xButton, True, 0);
                XFlush(g_display);
            }

            void mouseRelease(const Mouse& mouse) override
            {
                if (!g_display) return;
                const unsigned int xButton = toX11Button(mouse.button);
                if (xButton == 0) return;

                XTestFakeButtonEvent(g_display, xButton, False, 0);

                // Modifiers
                if (static_cast<bool>(mouse.modifier & KeyModifier::Meta)) XTestFakeKeyEvent(g_display, XKeysymToKeycode(g_display, XK_Super_L), False, 0);
                if (static_cast<bool>(mouse.modifier & KeyModifier::Alt)) XTestFakeKeyEvent(g_display, XKeysymToKeycode(g_display, XK_Alt_L), False, 0);
                if (static_cast<bool>(mouse.modifier & KeyModifier::Ctrl)) XTestFakeKeyEvent(g_display, XKeysymToKeycode(g_display, XK_Control_L), False, 0);
                if (static_cast<bool>(mouse.modifier & KeyModifier::Shift)) XTestFakeKeyEvent(g_display, XKeysymToKeycode(g_display, XK_Shift_L), False, 0);

                XFlush(g_display);
            }

            void keyDown(const Key& key) override
            {
                if (!g_display) return;
                const KeySym ks = toKeySym(key);
                if (ks == NoSymbol) return;
                XTestFakeKeyEvent(g_display, XKeysymToKeycode(g_display, ks), True, 0);
                XFlush(g_display);
            }

            void keyUp(const Key& key) override
            {
                if (!g_display) return;
                const KeySym ks = toKeySym(key);
                if (ks == NoSymbol) return;
                XTestFakeKeyEvent(g_display, XKeysymToKeycode(g_display, ks), False, 0);
                XFlush(g_display);
            }

            void mouseDown(const Mouse& mouse) override
            {
                if (!g_display) return;
                const unsigned int xButton = toX11Button(mouse.button);
                if (xButton == 0) return;
                XTestFakeButtonEvent(g_display, xButton, True, 0);
                XFlush(g_display);
            }

            void mouseUp(const Mouse& mouse) override
            {
                if (!g_display) return;
                const unsigned int xButton = toX11Button(mouse.button);
                if (xButton == 0) return;
                XTestFakeButtonEvent(g_display, xButton, False, 0);
                XFlush(g_display);
            }

            void moveMouseTo(int32_t x, int32_t y) override
            {
                if (!g_display) return;
                XTestFakeMotionEvent(g_display, -1, x, y, 0);
                XFlush(g_display);
            }

            std::pair<int32_t, int32_t> getCursorPosition() override
            {
                if (!g_display) return { 0, 0 };
                Window root, child;
                int rootX, rootY, winX, winY;
                unsigned int mask;
                if (XQueryPointer(g_display, g_rootWindow, &root, &child, &rootX, &rootY, &winX, &winY, &mask))
                {
                    return { rootX, rootY };
                }
                return { 0, 0 };
            }

            std::vector<AppWindowInfo> enumerateWindows() override
            {
                std::vector<AppWindowInfo> windows;
                if (!g_display) return windows;

                Window root_return, parent_return;
                Window* children_return;
                unsigned int nchildren_return;

                if (XQueryTree(g_display, g_rootWindow, &root_return, &parent_return, &children_return, &nchildren_return))
                {
                    for (unsigned int i = 0; i < nchildren_return; ++i)
                    {
                        XWindowAttributes attrs;
                        if (XGetWindowAttributes(g_display, children_return[i], &attrs) && attrs.map_state == IsViewable)
                        {
                            AppWindowInfo info = getX11AppWindowInfo(children_return[i]);
                            if (!info.windowTitle.empty() || !info.processName.empty())
                            {
                                windows.push_back(info);
                            }
                        }
                    }
                    if (children_return) XFree(children_return);
                }
                return windows;
            }

            std::optional<AppWindowInfo> getForegroundWindow() override
            {
                if (!g_display) return std::nullopt;
                Window focusedWindow;
                int revert_to;
                XGetInputFocus(g_display, &focusedWindow, &revert_to);
                if (focusedWindow == None || focusedWindow == PointerRoot) return std::nullopt;
                return getX11AppWindowInfo(focusedWindow);
            }

            BackendCapabilities capabilities() const override
            {
                return {
                    .keyboardHooks = true,
                    .mouseHooks = true,
                    .focusDetection = true,
                    .listApplications = true,
                    .syntheticKeyboardInput = true,
                    .syntheticMouseInput = true,
                    .absoluteMouseMovement = true,
                    .getCursorPosition = true
                };
            }
        };
    }

    int32_t getX11VirtualKey(const X11KeyboardData& data)
    {
        return static_cast<int32_t>(data.event.keycode - 8);
    }

    std::string getX11ActiveApplicationName()
    {
        if (!g_display) return "";

        Window focusedWindow;
        int revert_to;
        XGetInputFocus(g_display, &focusedWindow, &revert_to);

        if (focusedWindow == None || focusedWindow == PointerRoot) return "";

        XClassHint classHint;
        if (XGetClassHint(g_display, focusedWindow, &classHint))
        {
            std::string name = classHint.res_name ? classHint.res_name : "";
            if (classHint.res_name) XFree(classHint.res_name);
            if (classHint.res_class) XFree(classHint.res_class);
            return name;
        }

        return "";
    }

    std::vector<std::string> getX11RunningApplicationNames()
    {
        if (!g_display) return {};

        std::set<std::string> names;
        Window root_return, parent_return;
        Window* children_return;
        unsigned int nchildren_return;

        if (XQueryTree(g_display, g_rootWindow, &root_return, &parent_return, &children_return, &nchildren_return))
        {
            for (unsigned int i = 0; i < nchildren_return; ++i)
            {
                XClassHint classHint;
                if (XGetClassHint(g_display, children_return[i], &classHint))
                {
                    if (classHint.res_name)
                    {
                        names.insert(classHint.res_name);
                        XFree(classHint.res_name);
                    }
                    if (classHint.res_class) XFree(classHint.res_class);
                }
            }
            if (children_return) XFree(children_return);
        }

        return { names.begin(), names.end() };
    }

    std::unique_ptr<IPlatformBackend> createX11Backend()
    {
        return std::make_unique<X11Backend>();
    }

    bool isX11KeyDown(const X11KeyboardData& data) { return data.event.type == KeyPress; }
    bool isX11KeyUp(const X11KeyboardData& data) { return data.event.type == KeyRelease; }
    int8_t getX11Char(const X11KeyboardData& data)
    {
        if (data.event.type != KeyPress) return INVALID_KEY;
        KeySym keysym = XLookupKeysym(const_cast<XKeyEvent*>(&data.event), 0);
        if (keysym >= XK_a && keysym <= XK_z) return static_cast<int8_t>(keysym);
        if (keysym >= XK_A && keysym <= XK_Z) return static_cast<int8_t>(std::tolower(static_cast<int>(keysym)));
        if (keysym >= 0x20 && keysym <= 0x7e) return static_cast<int8_t>(keysym);
        return INVALID_KEY;
    }
    int64_t getX11FunctionKey(const X11KeyboardData& data)
    {
        if (data.event.type != KeyPress) return INVALID_KEY;
        KeySym keysym = XLookupKeysym(const_cast<XKeyEvent*>(&data.event), 0);
        if (keysym >= XK_F1 && keysym <= XK_F24)
        {
            return static_cast<int64_t>(keysym - XK_F1 + 1);
        }
        return INVALID_KEY;
    }
    void printX11KeyboardInfo(const X11KeyboardData& data)
    {
        if (!Logger::isDebugModeEnabled()) return;
        Logger::debug("[{}] keycode={} state={}\n", 
            data.event.type == KeyPress ? "DOWN" : "UP", 
            data.event.keycode, 
            data.event.state);
    }

    bool isX11LeftButtonDown(const X11MouseData& data) { return data.event.type == ButtonPress && data.event.button == 1; }
    bool isX11LeftButtonUp(const X11MouseData& data) { return data.event.type == ButtonRelease && data.event.button == 1; }
    bool isX11RightButtonDown(const X11MouseData& data) { return data.event.type == ButtonPress && data.event.button == 3; }
    bool isX11RightButtonUp(const X11MouseData& data) { return data.event.type == ButtonRelease && data.event.button == 3; }
    bool isX11MiddleButtonDown(const X11MouseData& data) { return data.event.type == ButtonPress && data.event.button == 2; }
    bool isX11MiddleButtonUp(const X11MouseData& data) { return data.event.type == ButtonRelease && data.event.button == 2; }
    bool isX11BackButtonDown(const X11MouseData& data) { return data.event.type == ButtonPress && data.event.button == 8; }
    bool isX11BackButtonUp(const X11MouseData& data) { return data.event.type == ButtonRelease && data.event.button == 8; }
    bool isX11ForwardButtonDown(const X11MouseData& data) { return data.event.type == ButtonPress && data.event.button == 9; }
    bool isX11ForwardButtonUp(const X11MouseData& data) { return data.event.type == ButtonRelease && data.event.button == 9; }
    void printX11MouseInfo(const X11MouseData& data)
    {
        if (!Logger::isDebugModeEnabled()) return;
        if (data.event.type != ButtonPress && data.event.type != ButtonRelease) return;
        Logger::debug("[{}] button={} state={}\n", 
            data.event.type == ButtonPress ? "DOWN" : "UP", 
            data.event.button, 
            data.event.state);
    }
}
