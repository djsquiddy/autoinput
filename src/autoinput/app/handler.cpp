/**
 * @file handler.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include "autoinput/support/types.h"
#include "autoinput/input/keyboard.h"
#include "autoinput/input/mouse.h"
#include "autoinput/platform/backend.h"

namespace autoinput
{
    void KeyHandler::press()
    {
        IPlatformBackend* backend = m_backend;
        if (!backend) return;
        if (bool expected = false; m_isPressed.compare_exchange_strong(expected, true))
        {
            std::shared_lock lock(m_keyMutex);
            backend->keyPress(m_key);
        }
    }

    void KeyHandler::release()
    {
        IPlatformBackend* backend = m_backend;
        if (!backend) return;
        if (bool expected = true; m_isPressed.compare_exchange_strong(expected, false))
        {
            std::shared_lock lock(m_keyMutex);
            backend->keyRelease(m_key);
        }
    }

    void MouseHandler::press()
    {
        IPlatformBackend* backend = m_backend;
        if (!backend) return;
        if (bool expected = false; m_isPressed.compare_exchange_strong(expected, true))
        {
            std::shared_lock lock(m_mouseMutex);
            backend->mousePress(m_mouse);
        }
    }

    void MouseHandler::release()
    {
        IPlatformBackend* backend = m_backend;
        if (!backend) return;
        if (bool expected = true; m_isPressed.compare_exchange_strong(expected, false))
        {
            std::shared_lock lock(m_mouseMutex);
            backend->mouseRelease(m_mouse);
        }
    }
}
