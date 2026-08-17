/**
 * @file win32TrayIcon.cpp
 * @author djsquiddy
 * @date August 2026
 */
#ifdef _WIN32

#undef NOGDI
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include "win32TrayIcon.h"
#include "autoinput/support/logger.h"
#include <stdexcept>
#include <vector>

#define WM_TRAYICON (WM_USER + 1)
#define IDI_APP_ICON 101

namespace autoinput::tray
{
    class GdiPlusInitializer
    {
    public:
        GdiPlusInitializer()
        {
            Gdiplus::GdiplusStartupInput gdiplusStartupInput;
            Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
        }
        ~GdiPlusInitializer()
        {
            Gdiplus::GdiplusShutdown(m_gdiplusToken);
        }
    private:
        ULONG_PTR m_gdiplusToken;
    };

    static HICON LoadPngAsIcon(int resourceId)
    {
        static GdiPlusInitializer initializer;

        HRSRC hResInfo = FindResource(GetModuleHandle(NULL), MAKEINTRESOURCE(resourceId), RT_RCDATA);
        if (!hResInfo) return NULL;

        DWORD dwSize = SizeofResource(GetModuleHandle(NULL), hResInfo);
        HGLOBAL hResData = LoadResource(GetModuleHandle(NULL), hResInfo);
        if (!hResData) return NULL;

        void* pResData = LockResource(hResData);
        if (!pResData) return NULL;

        HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, dwSize);
        if (!hBuffer) return NULL;

        void* pBuffer = GlobalLock(hBuffer);
        if (pBuffer)
        {
            CopyMemory(pBuffer, pResData, dwSize);
            GlobalUnlock(hBuffer);
        }

        IStream* pStream = NULL;
        if (CreateStreamOnHGlobal(hBuffer, TRUE, &pStream) != S_OK)
        {
            GlobalFree(hBuffer);
            return NULL;
        }

        Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromStream(pStream);
        HICON hIcon = NULL;
        if (bitmap)
        {
            bitmap->GetHICON(&hIcon);
            delete bitmap;
        }

        pStream->Release();
        return hIcon;
    }

    Win32TrayIcon::Win32TrayIcon(HWND hwnd) : m_hwnd(hwnd)
    {
        m_nid = {};
        m_nid.cbSize = sizeof(m_nid);
        m_nid.hWnd = hwnd;
        m_nid.uID = 1;
        m_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        m_nid.uCallbackMessage = WM_TRAYICON;
        
        m_nid.hIcon = LoadPngAsIcon(IDI_APP_ICON);
        if (m_nid.hIcon)
        {
            m_iconOwned = true;
        }
        else
        {
            m_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
            m_iconOwned = false;
        }
        
        strncpy_s(m_nid.szTip, "AutoInput", _TRUNCATE);
    }

    Win32TrayIcon::~Win32TrayIcon()
    {
        hide();
        if (m_nid.hIcon && m_iconOwned)
        {
            DestroyIcon(m_nid.hIcon);
            m_nid.hIcon = NULL;
        }
    }

    void Win32TrayIcon::show()
    {
        Shell_NotifyIconA(NIM_ADD, &m_nid);
    }

    void Win32TrayIcon::hide()
    {
        Shell_NotifyIconA(NIM_DELETE, &m_nid);
    }

    void Win32TrayIcon::setTooltip(const std::string& tooltip)
    {
        strncpy_s(m_nid.szTip, tooltip.c_str(), _TRUNCATE);
        Shell_NotifyIconA(NIM_MODIFY, &m_nid);
    }

    void Win32TrayIcon::setIcon(const std::string& iconResource)
    {
        HICON hNewIcon = LoadPngAsIcon(IDI_APP_ICON);
        bool newIconOwned = false;
        if (!hNewIcon)
        {
            hNewIcon = LoadIcon(NULL, IDI_APPLICATION);
            newIconOwned = false;
        }
        else
        {
            newIconOwned = true;
        }

        if (hNewIcon)
        {
            if (m_nid.hIcon && m_iconOwned)
            {
                DestroyIcon(m_nid.hIcon);
            }
            m_nid.hIcon = hNewIcon;
            m_iconOwned = newIconOwned;
            Shell_NotifyIconA(NIM_MODIFY, &m_nid);
        }
    }

    void Win32TrayIcon::setMenu(const std::vector<MenuItem>& items)
    {
        m_menuItems = items;
    }

    void Win32TrayIcon::showNotification(const std::string& title, const std::string& message)
    {
        Logger::info("Showing notification: [{}] {}", title, message);
        NOTIFYICONDATAA nid = m_nid;
        nid.uFlags |= NIF_INFO;
        strncpy_s(nid.szInfoTitle, title.c_str(), _TRUNCATE);
        strncpy_s(nid.szInfo, message.c_str(), _TRUNCATE);
        nid.dwInfoFlags = NIIF_INFO;
        Shell_NotifyIconA(NIM_MODIFY, &nid);
    }

    void Win32TrayIcon::update()
    {
        Shell_NotifyIconA(NIM_MODIFY, &m_nid);
    }

    void Win32TrayIcon::handleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_TRAYICON)
        {
            if (LOWORD(lParam) == WM_RBUTTONUP)
            {
                Logger::debug("Tray icon right-clicked, showing menu");
                POINT pt;
                GetCursorPos(&pt);

                HMENU hMenu = CreatePopupMenu();
                
                m_commandHandlers.clear();
                m_nextCommandId = 1001;
                createMenu(hMenu, m_menuItems);

                SetForegroundWindow(m_hwnd);
                TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hwnd, NULL);
                DestroyMenu(hMenu);
            }
        }
        else if (msg == WM_COMMAND)
        {
            UINT id = LOWORD(wParam);
            auto it = m_commandHandlers.find(id);
            if (it != m_commandHandlers.end() && it->second)
            {
                Logger::debug("Menu command triggered: {}", id);
                it->second();
            }
        }
    }

    void Win32TrayIcon::createMenu(HMENU hMenu, const std::vector<MenuItem>& items)
    {
        for (const auto& item : items)
        {
            if (item.isSeperator())
            {
                AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
                continue;
            }

            UINT flags = MF_STRING;
            if (!item.isEnabled()) flags |= MF_GRAYED;
            if (item.isChecked()) flags |= MF_CHECKED;

            if (!item.subItems.empty())
            {
                HMENU hSubMenu = CreatePopupMenu();
                createMenu(hSubMenu, item.subItems);
                AppendMenuA(hMenu, flags | MF_POPUP, (UINT_PTR)hSubMenu, item.text.c_str());
            }
            else
            {
                UINT id = m_nextCommandId++;
                m_commandHandlers[id] = item.action;
                AppendMenuA(hMenu, flags, id, item.text.c_str());
            }
        }
    }

    std::unique_ptr<ITrayIcon> createTrayIcon(void* nativeWindowHandle)
    {
        return std::make_unique<Win32TrayIcon>((HWND)nativeWindowHandle);
    }
}

#else

namespace autoinput::tray
{
    std::unique_ptr<ITrayIcon> createTrayIcon(void* nativeWindowHandle)
    {
        return nullptr;
    }
}

#endif // _WIN32
