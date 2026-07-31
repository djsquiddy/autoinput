/**
 * @file handler.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include "autoinput/types.h"
#include "autoinput/keyboard.h"
#include "autoinput/mouse.h"
#include "autoinput/backend.h"

namespace autoinput
{
    void KeyHandler::press()
    {
        if (!g_backend) return;
        if (bool expected = false; m_isPressed.compare_exchange_strong(expected, true))
        {
            std::shared_lock lock(m_keyMutex);
            g_backend->keyPress(m_key);
        }
    }

    void KeyHandler::release()
    {
        if (!g_backend) return;
        if (bool expected = true; m_isPressed.compare_exchange_strong(expected, false))
        {
            std::shared_lock lock(m_keyMutex);
            g_backend->keyRelease(m_key);
        }
    }

    void MouseHandler::press()
    {
        if (!g_backend) return;
        if (bool expected = false; m_isPressed.compare_exchange_strong(expected, true))
        {
            std::shared_lock lock(m_mouseMutex);
            g_backend->mousePress(m_mouse);
        }
    }

    void MouseHandler::release()
    {
        if (!g_backend) return;
        if (bool expected = true; m_isPressed.compare_exchange_strong(expected, false))
        {
            std::shared_lock lock(m_mouseMutex);
            g_backend->mouseRelease(m_mouse);
        }
    }
}
