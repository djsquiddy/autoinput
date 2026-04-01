//
// Created by djsquiddy on 3/9/2026.
//

#include <iostream>
#include <memory>

#include "autoinput/autoinput.h"
#include "autoinput/mouse.h"
#include "autoinput/keyboard.h"
#include "autoinput/types.h"
#include "autoinput/platform.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // defined(_WIN32)

namespace autoinput
{
    namespace platform
    {
        void signalEnd()
        {
            PostQuitMessage(0);
        }
    }

    std::string MouseHandler::getButtonName() const
    {
        return mouseButtonToString(m_mouseButton);
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
        std::cout << "Pressing " << mouseButtonToString(m_mouseButton) << " button" << std::endl;
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
        std::cout << "Releasing " << mouseButtonToString(m_mouseButton) << " button" << std::endl;
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

    bool KeyboardInput::isKeyDown() const {return data.wParam == WM_KEYDOWN || data.wParam == WM_SYSKEYDOWN;}
    bool KeyboardInput::isKeyUp() const {return data.wParam == WM_KEYUP || data.wParam == WM_SYSKEYUP;}
    bool KeyboardInput::isSysKey() const {return data.wParam == WM_SYSKEYDOWN  || data.wParam == WM_SYSKEYUP;}

    int32_t KeyboardInput::getChar() const
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

    int32_t KeyboardInput::functionKey() const
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
        std::string type = isKeyDown() ? "DOWN" : (isKeyUp() ? "UP" : "???");
        if (isSysKey()) type += " (sys)";

        // Print virtual key code + scan code
        std::cout << "[" << type << "] vk=0x" << std::hex << data.kbdStruct->vkCode
                  << " scan=0x" << data.kbdStruct->scanCode
                  << " flags=0x" << data.kbdStruct->flags;

        // Optional: human-readable char for printable keys (very rough)
        if (const char ch = getChar(); ch != INVALID_KEY)
        {
            std::cout << "  char='" << ch << "'";
        }
        else if (const int32_t fnKey = functionKey(); fnKey != -1)
        {
            std::cout << "  F=" << fnKey;
        }

        std::cout << std::endl;
    }

    /*
    *Quick usage notes

    Compile as console application (with /SUBSYSTEM:CONSOLE)
    Run as administrator if you want to reliably catch keys in elevated / UAC-protected apps (not always needed, but helps)
    To make it silent / background → hide console or make Windows service / tray app
    To simulate keys in response → call SendInput inside the callback (or elsewhere)
    */
#if AUTOINPUT_HOOK_KEYBOARD_ENABLED
    HHOOK g_hKeyboardHook = NULL;
#endif // AUTOINPUT_HOOK_KEYBOARD_ENABLED
#if AUTOINPUT_HOOK_MOUSE_ENABLED
    HHOOK g_hMouseHook = NULL;
#endif // AUTOINPUT_HOOK_MOUSE_ENABLED


#if AUTOINPUT_HOOK_KEYBOARD_ENABLED
    std::unique_ptr<KeyboardData> keyboardData;
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
            KeyboardInput input{ *keyboardData };
            input.printInfo();
            if (g_program->processKeyEvent(std::move(input)))
            {
                return 1;
            }

            // ───────────────────────────────────────────────
            // Want to BLOCK this key? Just return 1 here.
            // Example: block Windows key
            // if (p->vkCode == VK_LWIN || p->vkCode == VK_RWIN) return 1;
            // ───────────────────────────────────────────────
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
            bool didPrint = false;
            if (input.isLeftButtonDown())
            {
                std::cout << "Left button pressed";
                didPrint = true;
            }
            else if (input.isLeftButtonUp())
            {
                std::cout << "Left button released";
                didPrint = true;
            }
            if (input.isRightButtonDown())
            {
                std::cout << "Right button pressed";
                didPrint = true;
            }
            else if (input.isRightButtonUp())
            {
                std::cout << "Right button released";
                didPrint = true;
            }
            if (didPrint)
            {
                std::cout << std::endl;
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
            std::cerr << "SetWindowsHookEx for getting the Keyboard events failed: " << GetLastError() << "\n";
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
            std::cerr << "SetWindowsHookEx for getting the Mouse events failed: " << GetLastError() << "\n";
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