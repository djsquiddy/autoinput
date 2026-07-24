//
// Created by djsquiddy on 3/9/2026.
//

#include "autoinput/autoinput.h"
#include "autoinput/mouse.h"
#include "autoinput/keyboard.h"
#include "autoinput/logger.h"
#include "autoinput/types.h"
#include "autoinput/platform.h"

namespace autoinput
{
    namespace platform
    {
        void signalEnd()
        {
            PostQuitMessage(0);
        }
    }
   namespace
    {
        WORD toVirtualKey(const Key& key)
        {
            if (static_cast<bool>(key.modifier & KeyModifier::Function))
            {
                const int functionNumber = std::stoi(key.character);

                if (functionNumber < 1 || functionNumber > 24)
                {
                    throw std::invalid_argument{ "Function key must be between F1 and F24." };
                }

                return static_cast<WORD>(VK_F1 + functionNumber - 1);
            }

            if (key.character.length() != 1)
            {
                throw std::invalid_argument{ "Only single-character non-function keys are supported." };
            }

            const SHORT virtualKey = VkKeyScanExA(
                key.character.front(),
                GetKeyboardLayout(0)
            );

            if (virtualKey == -1)
            {
                throw std::invalid_argument{ "Unable to map character to virtual key." };
            }

            return LOBYTE(virtualKey);
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

        void sendKeyboardInputs(std::vector<INPUT>& inputs)
        {
            if (inputs.empty())
            {
                return;
            }

            const UINT sent = SendInput(
                static_cast<UINT>(inputs.size()),
                inputs.data(),
                sizeof(INPUT)
            );

            if (sent != inputs.size())
            {
                throw std::runtime_error{ "SendInput failed to send all keyboard inputs." };
            }
        }
    }

    void KeyHandler::pressKey()
    {
        if (m_isPressed)
        {
            return;
        }

        std::vector<INPUT> inputs;

        for (const std::vector<WORD> modifiers = modifierVirtualKeys(m_key.modifier); const WORD modifier : modifiers)
        {
            inputs.push_back(keyboardInput(modifier));
        }

        inputs.push_back(keyboardInput(toVirtualKey(m_key)));

#if AUTOINPUT_FAKE_HOOK
        sendKeyboardInputs(inputs);
#endif
        m_isPressed = true;
    }

    void KeyHandler::releaseKey()
    {
        if (!m_isPressed)
        {
            return;
        }

        std::vector<INPUT> inputs;
        const std::vector<WORD> modifiers = modifierVirtualKeys(m_key.modifier);

        inputs.push_back(keyboardInput(toVirtualKey(m_key), KEYEVENTF_KEYUP));

        for (auto it = modifiers.rbegin(); it != modifiers.rend(); ++it)
        {
            inputs.push_back(keyboardInput(*it, KEYEVENTF_KEYUP));
        }

#if AUTOINPUT_FAKE_HOOK
        sendKeyboardInputs(inputs);
#endif
        m_isPressed = false;
    }

    void MouseHandler::pressButton()
    {
        if (m_isButtonPressed)
        {
            return;
        }

        INPUT input{};
        input.type = INPUT_MOUSE;
        switch (m_mouseButton)
        {
        case MouseButton::LEFT:
            input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            break;
        case MouseButton::MIDDLE:
            input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
            break;
        case MouseButton::RIGHT:
            input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
            break;
        case MouseButton::BACK:
        case MouseButton::FORWARD:
        default:
            return;
        }
        Logger::debug("Pressing {} button\n", m_mouseButton);
#if AUTOINPUT_FAKE_HOOK
        SendInput(1, &input, sizeof(INPUT));
#endif
        m_isButtonPressed = true;
    }

    void MouseHandler::releaseButton()
    {
        if (!m_isButtonPressed)
        {
            return;
        }

        INPUT input{};
        input.type = INPUT_MOUSE;
        switch (m_mouseButton)
        {
        case MouseButton::LEFT:
            input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            break;
        case MouseButton::MIDDLE:
            input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
            break;
        case MouseButton::RIGHT:
            input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
            break;
        case MouseButton::BACK:
        case MouseButton::FORWARD:
        default:
            return;
        }
        Logger::debug("Releasing {} button\n", m_mouseButton);
#if AUTOINPUT_FAKE_HOOK
        SendInput(1, &input, sizeof(INPUT));
#endif
        m_isButtonPressed = false;
    }

#if AUTOINPUT_HOOK_MOUSE_ENABLED
    struct MouseInput
    {
        WPARAM wParam{ 0 };
        MSLLHOOKSTRUCT* mouseStruct{ nullptr };

        [[nodiscard]] bool isLeftButtonDown() const{  return wParam == WM_LBUTTONDOWN; }
        [[nodiscard]] bool isLeftButtonUp() const { return wParam == WM_LBUTTONUP; }
        [[nodiscard]] bool isRightButtonDown() const { return wParam == WM_RBUTTONDOWN; }
        [[nodiscard]] bool isRightButtonUp() const { return wParam == WM_RBUTTONUP; }
    };
#endif // AUTOINPUT_HOOK_MOUSE_ENABLED

    struct KeyboardData
    {
        WPARAM wParam{ 0 };
        KBDLLHOOKSTRUCT* kbdStruct{ nullptr };
    };

    KeyboardInput::KeyboardInput(KeyboardData& data) : data{ data } {}

    bool KeyboardInput::isKeyDown() const { return data.wParam == WM_KEYDOWN || data.wParam == WM_SYSKEYDOWN; }
    bool KeyboardInput::isKeyUp() const { return data.wParam == WM_KEYUP || data.wParam == WM_SYSKEYUP; }
    bool KeyboardInput::isSysKey() const { return data.wParam == WM_SYSKEYDOWN  || data.wParam == WM_SYSKEYUP; }

    int8_t KeyboardInput::getChar() const
    {
        if (!isKeyDown())
        {
            return INVALID_KEY;
        }
        if (data.kbdStruct->vkCode >= 0x30 && data.kbdStruct->vkCode <= 0x5A)
        {
            char ch = static_cast<char>(data.kbdStruct->vkCode);
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
        if (!isKeyDown())
        {
            return INVALID_KEY;
        }
        if (data.kbdStruct->vkCode >= VK_F1 && data.kbdStruct->vkCode <= VK_F24)
        {
            return static_cast<int32_t>(data.kbdStruct->vkCode - VK_F1) + 1;
        }
        return INVALID_KEY;
    }

    void KeyboardInput::printInfo() const
    {
        std::string type = isKeyDown() ? "DOWN" : isKeyUp() ? "UP" : "???";
        if (isSysKey())
        {
            type += " (sys)";
        }

        // Print virtual key code + scan code
        auto info = std::format("[{}] vk=0x{:x} scan=0x{:x} flags=0x{:x}",
                                 type,
                                 data.kbdStruct->vkCode,
                                 data.kbdStruct->scanCode,
                                 data.kbdStruct->flags);
        // Optional: human-readable char for printable keys (very rough)
        if (const auto ch = getChar(); ch != INVALID_KEY)
        {
            info += "  char='" + std::to_string(ch) + "'";
        }
        else if (const auto fnKey = functionKey(); fnKey != -1)
        {
            info += "  F=" + std::to_string(fnKey);
        }

        Logger::debug(info);
    }

#if AUTOINPUT_HOOK_KEYBOARD_ENABLED
    HHOOK g_hKeyboardHook = nullptr;
    std::unique_ptr<KeyboardData> keyboardData;
#endif // AUTOINPUT_HOOK_KEYBOARD_ENABLED
#if AUTOINPUT_HOOK_MOUSE_ENABLED
    HHOOK g_hMouseHook = nullptr;
#endif // AUTOINPUT_HOOK_MOUSE_ENABLED


#if AUTOINPUT_HOOK_KEYBOARD_ENABLED
    // Low-level keyboard callback
    LRESULT CALLBACK LowLevelKeyboardProc(
        const int    nCode,
        const WPARAM wParam,
        const LPARAM lParam)
    {
        if (nCode == HC_ACTION)        // Process this event
        {
            auto* p = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
            keyboardData->wParam = wParam;
            keyboardData->kbdStruct = p;
            if (g_program->processKeyEvent(KeyboardInput { *keyboardData }))
            {
                return 1;
            }
        }

        // Let the event continue normally
        return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
    }
#endif // AUTOINPUT_HOOK_KEYBOARD_ENABLED

#if AUTOINPUT_HOOK_MOUSE_ENABLED
    // Low-level mouse callback
    LRESULT CALLBACK LowLevelMouseProc(
        const int    nCode,
        const WPARAM wParam,
        const LPARAM lParam)
    {
        if (nCode == HC_ACTION)        // Process this event
        {
            auto* p = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
            const MouseInput input = {.wParam = wParam, .mouseStruct = p};
            if (input.isLeftButtonDown())
            {
                Logger::debug("Left button pressed");
            }
            else if (input.isLeftButtonUp())
            {
                Logger::debug("Left button released");
            }
            if (input.isRightButtonDown())
            {
                Logger::debug("Right button pressed");
            }
            else if (input.isRightButtonUp())
            {
                Logger::debug("Right button released");
            }
        }

        // Let the event continue normally
        return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
    }
#endif // AUTOINPUT_HOOK_MOUSE_ENABLED

    bool installHooks()
    {
#if AUTOINPUT_HOOK_KEYBOARD_ENABLED
        keyboardData = std::make_unique<KeyboardData>();
        // Install the global low-level keyboard hook
        g_hKeyboardHook = SetWindowsHookEx(
            WH_KEYBOARD_LL,             // hook type
            LowLevelKeyboardProc,       // callback
            GetModuleHandle(NULL),      // module (NULL = current exe)
            0                           // 0 = global
        );

        if (!g_hKeyboardHook)
        {
            Logger::error(std::format("SetWindowsHookEx for getting the Keyboard events failed: {}\n", GetLastError()));
            return false;
        }
#endif // AUTOINPUT_HOOK_KEYBOARD_ENABLED
#if AUTOINPUT_HOOK_MOUSE_ENABLED
        g_hMouseHook = SetWindowsHookEx(
            WH_MOUSE_LL,             // hook type
            LowLevelMouseProc,       // callback
            GetModuleHandle(NULL),      // module (NULL = current exe)
            0                           // 0 = global
        );

        if (!g_hMouseHook)
        {
            Logger::error(std::format("SetWindowsHookEx for getting the Mouse events failed: {}\n", GetLastError()));
            return false;
        }
#endif // AUTOINPUT_HOOK_MOUSE_ENABLED
        return true;
    }

    void runListener()
    {
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    void cleanup()
    {
#if AUTOINPUT_HOOK_KEYBOARD_ENABLED
        // Cleanup (rarely reached in console apps without signal handler)
        if (g_hKeyboardHook)
        {
            UnhookWindowsHookEx(g_hKeyboardHook);
        }
#endif // defined(HOOK_KEYBOARD_ENABLED) && HOOK_KEYBOARD_ENABLED

#if AUTOINPUT_HOOK_MOUSE_ENABLED
        if (g_hMouseHook)
        {
            UnhookWindowsHookEx(g_hMouseHook);
        }
#endif // AUTOINPUT_HOOK_MOUSE_ENABLED
    }

}