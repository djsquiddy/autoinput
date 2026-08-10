/**
 * @file win32TrayApp.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "win32TrayApp.h"
#include "win32TrayIcon.h"
#include "autoinput/support/logger.h"
#include <shellapi.h>

namespace autoinput::tray
{
    Win32TrayApp::Win32TrayApp()
        : TrayApp()
    {
    }

    Win32TrayApp::~Win32TrayApp()
    {
        shutdown();
        // ReSharper disable once CppZeroConstantCanBeReplacedWithNullptr
        UnregisterClassA("AutoInputTrayClass", GetModuleHandle(NULL)); // NOLINT(*-use-nullptr)
    }

    void Win32TrayApp::run()
    {
        WNDCLASSEXA wc = {0};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = WindowProc;
        // ReSharper disable once CppZeroConstantCanBeReplacedWithNullptr
        wc.hInstance = GetModuleHandle(NULL); // NOLINT(*-use-nullptr)
        wc.lpszClassName = "AutoInputTrayClass";
        RegisterClassExA(&wc);

        // ReSharper disable once CppZeroConstantCanBeReplacedWithNullptr
        m_hwnd = CreateWindowExA(0, wc.lpszClassName, "AutoInputTray", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, wc.hInstance, this); // NOLINT(*-use-nullptr)
        if (!m_hwnd)
        {
            Logger::error("Failed to create tray application window");
            return;
        }

        Logger::info("Tray application window created");
        m_trayIcon = createTrayIcon(m_hwnd);
        m_trayIcon->show();
        updateMenu();

        Logger::info("Entering message loop");
        MSG msg;
        while (m_running)
        {
            // ReSharper disable once CppZeroConstantCanBeReplacedWithNullptr
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) // NOLINT(*-use-nullptr)
            {
                if (msg.message == WM_QUIT)
                {
                    m_running = false;
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            processUIUpdates();
            if (m_running)
            {
                // ReSharper disable once CppZeroConstantCanBeReplacedWithNullptr
                MsgWaitForMultipleObjects(0, NULL, FALSE, 100, QS_ALLINPUT); // NOLINT(*-use-nullptr)
            }
        }
        Logger::info("Exited message loop");
    }

    void Win32TrayApp::shutdown()
    {
        TrayApp::shutdown();
        if (m_hwnd)
        {
            DestroyWindow(m_hwnd);
            // ReSharper disable once CppZeroConstantCanBeReplacedWithNullptr
            m_hwnd = NULL; // NOLINT(*-use-nullptr)
        }
        PostQuitMessage(0);
    }

    void Win32TrayApp::openConfigFolder()
    {
        const auto path = getUserConfigsPath(*m_environment);
        // ReSharper disable once CppZeroConstantCanBeReplacedWithNullptr
        ShellExecuteA(NULL, "open", path.string().c_str(), NULL, NULL, SW_SHOWNORMAL); // NOLINT(*-use-nullptr)
    }

    void Win32TrayApp::wakeUIThread()
    {
        if (m_hwnd)
        {
            PostMessage(m_hwnd, WM_TRAY_UPDATE_UI, 0, 0);
        }
    }

    LRESULT CALLBACK Win32TrayApp::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (uMsg == WM_NCCREATE)
        {
            auto* cs = reinterpret_cast<CREATESTRUCTA*>(lParam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        }
        auto* app = reinterpret_cast<Win32TrayApp*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if (app)
        {
            if (uMsg == WM_TRAY_UPDATE_UI)
            {
                app->processUIUpdates();
                return 0;
            }
            app->handleMessage(uMsg, wParam, lParam);
        }
        if (uMsg == WM_DESTROY)
        {
            return 0;
        }
        return DefWindowProcA(hwnd, uMsg, wParam, lParam);
    }

    void Win32TrayApp::handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (m_trayIcon)
        {
            // We know it's a Win32TrayIcon on Windows
            auto* win32Icon = static_cast<Win32TrayIcon*>(m_trayIcon.get());
            win32Icon->handleMessage(uMsg, wParam, lParam);
        }
    }

    std::unique_ptr<TrayApp> createTrayApp()
    {
        return std::make_unique<Win32TrayApp>();
    }
}
