/**
 * @file autoInput_win32.cpp
 * @author djsquiddy
 * @date March 2026
 */
#include "autoinput/autoInput.h"
#include "autoinput/mouse.h"
#include "autoinput/keyboard.h"
#include "autoinput/logger.h"
#include "autoinput/types.h"
#include "autoinput/platform.h"
#include "autoinput/backend.h"
#include "autoinput/win32/internalData_win32.h"
#include <set>
#include <ranges>
#include <unordered_map>
#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>
#include <cstdlib>

namespace autoinput
{
    namespace
    {
        WORD toVirtualKey(const Key& key);
    }

    namespace platform
    {
        namespace
        {
            DWORD g_mainThreadId = 0;

            BOOL WINAPI ConsoleHandler(const DWORD dwType)
            {
                if (dwType == CTRL_C_EVENT)
                {
                    signalEnd();
                    return TRUE;
                }
                return FALSE;
            }
        }

        void signalEnd()
        {
            IPlatformBackend* backend = g_program ? g_program->getBackend() : nullptr;
            if (backend)
            {
                backend->cleanup();
            }
            
            if (g_mainThreadId != 0 && GetCurrentThreadId() != g_mainThreadId)
            {
                PostThreadMessage(g_mainThreadId, WM_QUIT, 0, 0);
            }
            else
            {
                PostQuitMessage(0);
            }
        }

        void setupSignalHandler()
        {
            g_mainThreadId = GetCurrentThreadId();
            if (SetConsoleCtrlHandler(ConsoleHandler, TRUE) == FALSE)
            {
                Logger::fatal("Unable to install signal handler!\n");
            }
        }

        int32_t getVirtualKey(const Key& key)
        {
            try
            {
                return toVirtualKey(key);
            }
            catch (...)
            {
                return 0;
            }
        }

        std::string getActiveApplicationName()
        {
            const HWND hwnd = GetForegroundWindow();
            if (hwnd == nullptr)
            {
                return "";
            }

            DWORD processId;
            GetWindowThreadProcessId(hwnd, &processId);

            const HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
            if (hProcess == nullptr)
            {
                return "";
            }

            char processName[MAX_PATH];
            // ReSharper disable once CppZeroConstantCanBeReplacedWithNullptr
            if (GetModuleBaseNameA(hProcess, NULL, processName, sizeof(processName))) // NOLINT(*-use-nullptr)
            {
                CloseHandle(hProcess);
                return { processName };
            }

            CloseHandle(hProcess);
            return "";
        }

        std::vector<std::string> getRunningApplicationNames()
        {
            std::set<std::string> names;
            auto callback = [](HWND hwnd, LPARAM lParam) -> BOOL
            {
                if (!IsWindowVisible(hwnd))
                {
                    return TRUE;
                }

                DWORD processId;
                GetWindowThreadProcessId(hwnd, &processId);

                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
                if (hProcess == nullptr)
                {
                    return TRUE;
                }

                char processName[MAX_PATH];
                // ReSharper disable once CppZeroConstantCanBeReplacedWithNullptr
                if (GetModuleBaseNameA(hProcess, NULL, processName, sizeof(processName))) // NOLINT(*-use-nullptr)
                {
                    reinterpret_cast<std::set<std::string>*>(lParam)->insert(std::string(processName));
                }

                CloseHandle(hProcess);
                return TRUE;
            };

            EnumWindows(callback, reinterpret_cast<LPARAM>(&names));

            return { names.begin(), names.end() };
        }

        std::filesystem::path getExecutablePath()
        {
            char buffer[MAX_PATH];
            // ReSharper disable once CppZeroConstantCanBeReplacedWithNullptr
            GetModuleFileNameA(NULL, buffer, MAX_PATH);
            return std::filesystem::path(buffer).parent_path();
        }

        std::filesystem::path getUserHomePath()
        {
            if (const char* userProfile = std::getenv("USERPROFILE"))
            {
                return { userProfile };
            }
            return {};
        }
    }

    namespace
    {
        HHOOK g_hKeyboardHook = nullptr;
        std::unique_ptr<KeyboardData> g_keyboardData;
        HHOOK g_hMouseHook = nullptr;
        std::unique_ptr<MouseData> g_mouseData;
        HWINEVENTHOOK g_hFocusHook = nullptr;

        void CALLBACK WinEventProc(
            HWINEVENTHOOK hWinEventHook,
            DWORD event,
            HWND hwnd,
            LONG idObject,
            LONG idChild,
            DWORD dwEventThread,
            DWORD dwmsEventTime
        )
        {
            if (event == EVENT_SYSTEM_FOREGROUND && g_program)
            {
                g_program->onFocusChanged(platform::getActiveApplicationName());
            }
        }

        WORD getVirtualKeyFromString(const std::string& keyStr)
        {
            static const std::unordered_map<std::string, WORD> specialKeys = {
                {"esc", VK_ESCAPE},
                {"escape", VK_ESCAPE},
                {"space", VK_SPACE},
                {"tab", VK_TAB},
                {"enter", VK_RETURN},
                {"return", VK_RETURN},
                {"backspace", VK_BACK},
                {"ins", VK_INSERT},
                {"insert", VK_INSERT},
                {"del", VK_DELETE},
                {"delete", VK_DELETE},
                {"home", VK_HOME},
                {"end", VK_END},
                {"pageup", VK_PRIOR},
                {"pgup", VK_PRIOR},
                {"pagedown", VK_NEXT},
                {"pgdn", VK_NEXT},
                {"up", VK_UP},
                {"down", VK_DOWN},
                {"left", VK_LEFT},
                {"right", VK_RIGHT},
                {"capslock", VK_CAPITAL},
                {"numlock", VK_NUMLOCK},
                {"scrolllock", VK_SCROLL},
                {"printscreen", VK_SNAPSHOT},
                {"prtsc", VK_SNAPSHOT},
                {"pause", VK_PAUSE}
            };

            if (const auto it = specialKeys.find(keyStr); it != specialKeys.end())
            {
                return it->second;
            }
            return 0;
        }

        WORD toVirtualKey(const Key& key)
        {
            if (static_cast<bool>(key.modifier & KeyModifier::Function))
            {
                const int functionNumber = std::stoi(key.character);

                if (functionNumber < 1 || functionNumber > 24)
                {
                    throw std::invalid_argument{ "Function key must be between F1 and F24." };
                }

                return gsl::narrow_cast<WORD>(VK_F1 + functionNumber - 1);
            }

            const WORD specialVk = getVirtualKeyFromString(key.character);
            if (specialVk != 0)
            {
                return specialVk;
            }

            if (key.character.length() != 1)
            {
                throw std::invalid_argument{ "Unsupported key: " + key.character };
            }

            const SHORT virtualKey = VkKeyScanExA(
                key.character.front(),
                GetKeyboardLayout(0)
            );

            if (virtualKey == -1)
            {
                throw std::invalid_argument{ "Unable to map character to virtual key." };
            }

            return gsl::narrow_cast<BYTE>(virtualKey);
        }

        std::vector<WORD> modifierVirtualKeys(const KeyModifier modifier)
        {
            std::vector<WORD> keys;

            if (static_cast<bool>(modifier & KeyModifier::Ctrl))
            {
                keys.push_back(VK_CONTROL);
            }

            if (static_cast<bool>(modifier & KeyModifier::Shift))
            {
                keys.push_back(VK_SHIFT);
            }

            if (static_cast<bool>(modifier & KeyModifier::Alt))
            {
                keys.push_back(VK_MENU);
            }

            if (static_cast<bool>(modifier & KeyModifier::Meta))
            {
                keys.push_back(VK_LWIN);
            }

            return keys;
        }

        INPUT keyboardInput(const WORD virtualKey, const DWORD flags = 0)
        {
            INPUT input{};
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = virtualKey;
            input.ki.dwFlags = flags;
            return input;
        }

        void sendKeyboardInputs(const gsl::span<INPUT> inputs)
        {
            if (inputs.empty())
            {
                return;
            }

            const UINT sent = SendInput(
                gsl::narrow_cast<UINT>(inputs.size()),
                inputs.data(),
                sizeof(INPUT)
            );

            if (sent != inputs.size())
            {
                throw std::runtime_error{ "SendInput failed to send all keyboard inputs." };
            }
        }

        // Low-level keyboard callback
        LRESULT CALLBACK LowLevelKeyboardProc(
            const int    nCode,
            const WPARAM wParam,
            const LPARAM lParam)
        {
            if (nCode == HC_ACTION)        // Process this event
            {
                auto* p = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
                WindowsKeyboardData winData;
                winData.wParam = wParam;
                winData.kbdStruct = p;
                g_keyboardData->internal = winData;
                if (g_program->processKeyEvent(KeyboardInput { *g_keyboardData }))
                {
                    return 1;
                }
            }

            // Let the event continue normally
            return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
        }

        // Low-level mouse callback
        LRESULT CALLBACK LowLevelMouseProc(
            const int    nCode,
            const WPARAM wParam,
            const LPARAM lParam)
        {
            if (nCode != HC_ACTION)        // Process this event
            {
                // Let the event continue normally
                return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
            }

            auto* p = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
            WindowsMouseData winData;
            winData.wParam = wParam;
            winData.mouseStruct = p;
            g_mouseData->internal = winData;

            if (g_program->processMouseEvent(MouseInput { *g_mouseData }))
            {
                return 1;
            }

            // Let the event continue normally
            return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
        }

        class WindowsBackend : public IPlatformBackend
        {
        public:
            bool installHooks() override
            {
                g_keyboardData = std::make_unique<KeyboardData>();
                g_hKeyboardHook = SetWindowsHookEx(
                    WH_KEYBOARD_LL,
                    LowLevelKeyboardProc,
                    GetModuleHandle(NULL),
                    0
                );

                if (!g_hKeyboardHook)
                {
                    Logger::error("SetWindowsHookEx for Keyboard failed: {}\n", GetLastError());
                    return false;
                }

                g_mouseData = std::make_unique<MouseData>();
                g_hMouseHook = SetWindowsHookEx(
                    WH_MOUSE_LL,
                    LowLevelMouseProc,
                    GetModuleHandle(NULL),
                    0
                );

                if (!g_hMouseHook)
                {
                    Logger::error("SetWindowsHookEx for Mouse failed: {}\n", GetLastError());
                    return false;
                }

                g_hFocusHook = SetWinEventHook(
                    EVENT_SYSTEM_FOREGROUND,
                    EVENT_SYSTEM_FOREGROUND,
                    NULL,
                    WinEventProc,
                    0,
                    0,
                    WINEVENT_OUTOFCONTEXT
                );

                if (!g_hFocusHook)
                {
                    Logger::error("SetWinEventHook for Focus failed: {}\n", GetLastError());
                    return false;
                }

                if (g_program)
                {
                    g_program->onFocusChanged(platform::getActiveApplicationName());
                }

                return true;
            }

            void runListener() override
            {
                MSG msg;
                while (GetMessage(&msg, NULL, 0, 0))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }

            void cleanup() override
            {
                if (g_hKeyboardHook)
                {
                    UnhookWindowsHookEx(g_hKeyboardHook);
                    g_hKeyboardHook = nullptr;
                }
                if (g_hMouseHook)
                {
                    UnhookWindowsHookEx(g_hMouseHook);
                    g_hMouseHook = nullptr;
                }
                if (g_hFocusHook)
                {
                    UnhookWinEvent(g_hFocusHook);
                    g_hFocusHook = nullptr;
                }
            }

            void keyPress(const Key& key) override
            {
                std::vector<INPUT> inputs;
                const std::vector<WORD> modifiers = modifierVirtualKeys(key.modifier);
                inputs.reserve(modifiers.size() + 1);
                for (const WORD modifier : modifiers)
                {
                    inputs.push_back(keyboardInput(modifier));
                }
                inputs.push_back(keyboardInput(toVirtualKey(key)));
                sendKeyboardInputs(inputs);
            }

            void keyRelease(const Key& key) override
            {
                std::vector<INPUT> inputs;
                const std::vector<WORD> modifiers = modifierVirtualKeys(key.modifier);
                inputs.push_back(keyboardInput(toVirtualKey(key), KEYEVENTF_KEYUP));
                for (const auto modifier : std::views::reverse(modifiers))
                {
                    inputs.push_back(keyboardInput(modifier, KEYEVENTF_KEYUP));
                }
                sendKeyboardInputs(inputs);
            }

            void mousePress(const Mouse& mouse) override
            {
                std::vector<INPUT> inputs;
                const std::vector<WORD> modifiers = modifierVirtualKeys(mouse.modifier);
                inputs.reserve(modifiers.size() + 1);
                for (const WORD modifier : modifiers)
                {
                    inputs.push_back(keyboardInput(modifier));
                }

                INPUT mouseInput{};
                mouseInput.type = INPUT_MOUSE;
                switch (mouse.button)
                {
                case MouseButton::Left: mouseInput.mi.dwFlags = MOUSEEVENTF_LEFTDOWN; break;
                case MouseButton::Middle: mouseInput.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN; break;
                case MouseButton::Right: mouseInput.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN; break;
                case MouseButton::Back:
                    mouseInput.mi.dwFlags = MOUSEEVENTF_XDOWN;
                    mouseInput.mi.mouseData = XBUTTON1;
                    break;
                case MouseButton::Forward:
                    mouseInput.mi.dwFlags = MOUSEEVENTF_XDOWN;
                    mouseInput.mi.mouseData = XBUTTON2;
                    break;
                default: break;
                }

                if (mouseInput.mi.dwFlags != 0)
                {
                    inputs.push_back(mouseInput);
                }

                sendKeyboardInputs(inputs);
            }

            void mouseRelease(const Mouse& mouse) override
            {
                std::vector<INPUT> inputs;

                INPUT mouseInput{};
                mouseInput.type = INPUT_MOUSE;
                switch (mouse.button)
                {
                case MouseButton::Left: mouseInput.mi.dwFlags = MOUSEEVENTF_LEFTUP; break;
                case MouseButton::Middle: mouseInput.mi.dwFlags = MOUSEEVENTF_MIDDLEUP; break;
                case MouseButton::Right: mouseInput.mi.dwFlags = MOUSEEVENTF_RIGHTUP; break;
                case MouseButton::Back:
                    mouseInput.mi.dwFlags = MOUSEEVENTF_XUP;
                    mouseInput.mi.mouseData = XBUTTON1;
                    break;
                case MouseButton::Forward:
                    mouseInput.mi.dwFlags = MOUSEEVENTF_XUP;
                    mouseInput.mi.mouseData = XBUTTON2;
                    break;
                default: break;
                }

                if (mouseInput.mi.dwFlags != 0)
                {
                    inputs.push_back(mouseInput);
                }

                const std::vector<WORD> modifiers = modifierVirtualKeys(mouse.modifier);
                for (const auto modifier : std::views::reverse(modifiers))
                {
                    inputs.push_back(keyboardInput(modifier, KEYEVENTF_KEYUP));
                }

                sendKeyboardInputs(inputs);
            }

            void keyDown(const Key& key) override
            {
                std::vector<INPUT> inputs;
                inputs.push_back(keyboardInput(toVirtualKey(key)));
                sendKeyboardInputs(inputs);
            }

            void keyUp(const Key& key) override
            {
                std::vector<INPUT> inputs;
                inputs.push_back(keyboardInput(toVirtualKey(key), KEYEVENTF_KEYUP));
                sendKeyboardInputs(inputs);
            }

            void mouseDown(const Mouse& mouse) override
            {
                INPUT mouseInput{};
                mouseInput.type = INPUT_MOUSE;
                switch (mouse.button)
                {
                case MouseButton::Left: mouseInput.mi.dwFlags = MOUSEEVENTF_LEFTDOWN; break;
                case MouseButton::Middle: mouseInput.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN; break;
                case MouseButton::Right: mouseInput.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN; break;
                case MouseButton::Back:
                    mouseInput.mi.dwFlags = MOUSEEVENTF_XDOWN;
                    mouseInput.mi.mouseData = XBUTTON1;
                    break;
                case MouseButton::Forward:
                    mouseInput.mi.dwFlags = MOUSEEVENTF_XDOWN;
                    mouseInput.mi.mouseData = XBUTTON2;
                    break;
                default: break;
                }
                if (mouseInput.mi.dwFlags != 0)
                {
                    INPUT inputs[] = { mouseInput };
                    sendKeyboardInputs(inputs);
                }
            }

            void mouseUp(const Mouse& mouse) override
            {
                INPUT mouseInput{};
                mouseInput.type = INPUT_MOUSE;
                switch (mouse.button)
                {
                case MouseButton::Left: mouseInput.mi.dwFlags = MOUSEEVENTF_LEFTUP; break;
                case MouseButton::Middle: mouseInput.mi.dwFlags = MOUSEEVENTF_MIDDLEUP; break;
                case MouseButton::Right: mouseInput.mi.dwFlags = MOUSEEVENTF_RIGHTUP; break;
                case MouseButton::Back:
                    mouseInput.mi.dwFlags = MOUSEEVENTF_XUP;
                    mouseInput.mi.mouseData = XBUTTON1;
                    break;
                case MouseButton::Forward:
                    mouseInput.mi.dwFlags = MOUSEEVENTF_XUP;
                    mouseInput.mi.mouseData = XBUTTON2;
                    break;
                default: break;
                }
                if (mouseInput.mi.dwFlags != 0)
                {
                    INPUT inputs[] = { mouseInput };
                    sendKeyboardInputs(inputs);
                }
            }

            void moveMouseTo(int32_t x, int32_t y) override
            {
                INPUT mouseInput{};
                mouseInput.type = INPUT_MOUSE;
                mouseInput.mi.dx = (x * 65536) / GetSystemMetrics(SM_CXSCREEN);
                mouseInput.mi.dy = (y * 65536) / GetSystemMetrics(SM_CYSCREEN);
                mouseInput.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
                
                INPUT inputs[] = { mouseInput };
                sendKeyboardInputs(inputs);
            }

            std::pair<int32_t, int32_t> getCursorPosition() override
            {
                POINT p;
                if (GetCursorPos(&p))
                {
                    return { static_cast<int32_t>(p.x), static_cast<int32_t>(p.y) };
                }
                return { 0, 0 };
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

    std::unique_ptr<IPlatformBackend> createWindowsBackend()
    {
        return std::make_unique<WindowsBackend>();
    }


    MouseInput::MouseInput(MouseData& data) : data{ data } {}

    bool MouseInput::isLeftButtonDown() const 
    {
        const auto* winData = std::any_cast<WindowsMouseData>(&data.internal);
        return winData && winData->wParam == WM_LBUTTONDOWN; 
    }
    bool MouseInput::isLeftButtonUp() const 
    { 
        const auto* winData = std::any_cast<WindowsMouseData>(&data.internal);
        return winData && winData->wParam == WM_LBUTTONUP; 
    }
    bool MouseInput::isRightButtonDown() const 
    { 
        const auto* winData = std::any_cast<WindowsMouseData>(&data.internal);
        return winData && winData->wParam == WM_RBUTTONDOWN; 
    }
    bool MouseInput::isRightButtonUp() const 
    { 
        const auto* winData = std::any_cast<WindowsMouseData>(&data.internal);
        return winData && winData->wParam == WM_RBUTTONUP; 
    }
    bool MouseInput::isBackButtonDown() const 
    { 
        const auto* winData = std::any_cast<WindowsMouseData>(&data.internal);
        return winData && winData->wParam == WM_XBUTTONDOWN && HIWORD(winData->mouseStruct->mouseData) == XBUTTON1;
    }
    bool MouseInput::isBackButtonUp() const 
    { 
        const auto* winData = std::any_cast<WindowsMouseData>(&data.internal);
        return winData && winData->wParam == WM_XBUTTONUP && HIWORD(winData->mouseStruct->mouseData) == XBUTTON1;
    }
    bool MouseInput::isForwardButtonUp() const 
    { 
        const auto* winData = std::any_cast<WindowsMouseData>(&data.internal);
        return winData && winData->wParam == WM_XBUTTONUP && HIWORD(winData->mouseStruct->mouseData) == XBUTTON2;
    }
    bool MouseInput::isForwardButtonDown() const 
    { 
        const auto* winData = std::any_cast<WindowsMouseData>(&data.internal);
        return winData && winData->wParam == WM_XBUTTONDOWN && HIWORD(winData->mouseStruct->mouseData) == XBUTTON2;
    }
    bool MouseInput::isMiddleButtonUp() const 
    { 
        const auto* winData = std::any_cast<WindowsMouseData>(&data.internal);
        return winData && winData->wParam == WM_MBUTTONUP; 
    }
    bool MouseInput::isMiddleButtonDown() const 
    { 
        const auto* winData = std::any_cast<WindowsMouseData>(&data.internal);
        return winData && winData->wParam == WM_MBUTTONDOWN; 
    }
    bool MouseInput::isSynthetic() const
    {
        const auto* winData = std::any_cast<WindowsMouseData>(&data.internal);
        return winData && (winData->mouseStruct->flags & LLMHF_INJECTED);
    }
    bool MouseInput::isMouseMove() const
    {
        const auto* winData = std::any_cast<WindowsMouseData>(&data.internal);
        return winData && winData->wParam == WM_MOUSEMOVE;
    }

    MouseInput::ButtonState MouseInput::getButtonState() const
    {
        if (isLeftButtonDown()) return { MouseButton::Left, true };
        if (isLeftButtonUp()) return { MouseButton::Left, false };
        if (isRightButtonDown()) return { MouseButton::Right, true };
        if (isRightButtonUp()) return { MouseButton::Right, false };
        if (isMiddleButtonDown()) return { MouseButton::Middle, true };
        if (isMiddleButtonUp()) return { MouseButton::Middle, false };
        if (isBackButtonDown()) return { MouseButton::Back, true };
        if (isBackButtonUp()) return { MouseButton::Back, false };
        if (isForwardButtonDown()) return { MouseButton::Forward, true };
        if (isForwardButtonUp()) return { MouseButton::Forward, false };
        return { MouseButton::None, false };
    }

    void MouseInput::printInfo() const
    {
        if (!Logger::isDebugModeEnabled())
        {
            return;
        }

        std::string type = "???";
        std::string button = "None";

        if (isLeftButtonDown()) { type = "DOWN"; button = "Left"; }
        else if (isLeftButtonUp()) { type = "UP"; button = "Left"; }
        else if (isRightButtonDown()) { type = "DOWN"; button = "Right"; }
        else if (isRightButtonUp()) { type = "UP"; button = "Right"; }
        else if (isMiddleButtonDown()) { type = "DOWN"; button = "Middle"; }
        else if (isMiddleButtonUp()) { type = "UP"; button = "Middle"; }
        else if (isBackButtonDown()) { type = "DOWN"; button = "Back"; }
        else if (isBackButtonUp()) { type = "UP"; button = "Back"; }
        else if (isForwardButtonDown()) { type = "DOWN"; button = "Forward"; }
        else if (isForwardButtonUp()) { type = "UP"; button = "Forward"; }
        else
        {
            return;
        }

        Logger::debug("[{}] Mouse {} button\n", type, button);
    }

    KeyboardInput::KeyboardInput(KeyboardData& data) : data{ data } {}

    bool KeyboardInput::isKeyDown() const 
    {
        const auto* winData = std::any_cast<WindowsKeyboardData>(&data.internal);
        return winData && (winData->wParam == WM_KEYDOWN || winData->wParam == WM_SYSKEYDOWN); 
    }
    bool KeyboardInput::isKeyUp() const 
    { 
        const auto* winData = std::any_cast<WindowsKeyboardData>(&data.internal);
        return winData && (winData->wParam == WM_KEYUP || winData->wParam == WM_SYSKEYUP); 
    }
    bool KeyboardInput::isSysKey() const 
    { 
        const auto* winData = std::any_cast<WindowsKeyboardData>(&data.internal);
        return winData && (winData->wParam == WM_SYSKEYDOWN || winData->wParam == WM_SYSKEYUP); 
    }
    bool KeyboardInput::isSynthetic() const
    {
        const auto* winData = std::any_cast<WindowsKeyboardData>(&data.internal);
        return winData && (winData->kbdStruct->flags & LLKHF_INJECTED);
    }

    int8_t KeyboardInput::getChar() const
    {
        const auto* winData = std::any_cast<WindowsKeyboardData>(&data.internal);
        if (winData && winData->kbdStruct->vkCode >= 0x30 && winData->kbdStruct->vkCode <= 0x5A)
        {
            char ch = static_cast<char>(winData->kbdStruct->vkCode);
            if (!(GetKeyState(VK_SHIFT) & 0x8000))
            {
                ch += 32; // crude lowercase
            }
            return ch;
        }
        return INVALID_KEY;
    }

    int64_t KeyboardInput::functionKey() const
    {
        const auto* winData = std::any_cast<WindowsKeyboardData>(&data.internal);
        if (winData && winData->kbdStruct->vkCode >= VK_F1 && winData->kbdStruct->vkCode <= VK_F24)
        {
            return static_cast<int32_t>(winData->kbdStruct->vkCode - VK_F1) + 1;
        }
        return INVALID_KEY;
    }

    KeyState KeyboardInput::getKeyState() const
    {
        const auto* winData = std::any_cast<WindowsKeyboardData>(&data.internal);
        int32_t vk = winData ? static_cast<int32_t>(winData->kbdStruct->vkCode) : 0;
        return { static_cast<int32_t>(getChar()), static_cast<int32_t>(functionKey()), vk, KeyModifier::None };
    }

    void KeyboardInput::printInfo() const
    {
        if (!Logger::isDebugModeEnabled())
        {
            return;
        }

        const auto* winData = std::any_cast<WindowsKeyboardData>(&data.internal);
        if (!winData) return;

        std::string type = isKeyDown() ? "DOWN" : isKeyUp() ? "UP" : "???";
        if (isSysKey())
        {
            type += " (sys)";
        }

        auto info = std::format("[{}] vk=0x{:x} scan=0x{:x} flags=0x{:x}",
                                 type,
                                 winData->kbdStruct->vkCode,
                                 winData->kbdStruct->scanCode,
                                 winData->kbdStruct->flags);
        if (const auto ch = getChar(); ch != INVALID_KEY)
        {
            info += "  char='" + std::to_string(ch) + "'";
        }
        else if (const auto fnKey = functionKey(); fnKey != -1)
        {
            info += "  F=" + std::to_string(fnKey);
        }

        Logger::debug(info + "\n");
    }
}
