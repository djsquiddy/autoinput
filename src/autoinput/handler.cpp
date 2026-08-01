/**
 * @file handler.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include "autoinput/types.h"
#include "autoinput/keyboard.h"
#include "autoinput/mouse.h"
#include "autoinput/backend.h"
#include "autoinput/backendContext.h"

namespace autoinput
{
    void KeyHandler::press()
    {
        IPlatformBackend* backend = BackendRegistry::getBackend();
        if (!backend) return;
        if (bool expected = false; m_isPressed.compare_exchange_strong(expected, true))
        {
            std::shared_lock lock(m_keyMutex);
            backend->keyPress(m_key);
        }
    }

    void KeyHandler::release()
    {
        IPlatformBackend* backend = BackendRegistry::getBackend();
        if (!backend) return;
        if (bool expected = true; m_isPressed.compare_exchange_strong(expected, false))
        {
            std::shared_lock lock(m_keyMutex);
            backend->keyRelease(m_key);
        }
    }

    void MouseHandler::press()
    {
        IPlatformBackend* backend = BackendRegistry::getBackend();
        if (!backend) return;
        if (bool expected = false; m_isPressed.compare_exchange_strong(expected, true))
        {
            std::shared_lock lock(m_mouseMutex);
            backend->mousePress(m_mouse);
        }
    }

    void MouseHandler::release()
    {
        IPlatformBackend* backend = BackendRegistry::getBackend();
        if (!backend) return;
        if (bool expected = true; m_isPressed.compare_exchange_strong(expected, false))
        {
            std::shared_lock lock(m_mouseMutex);
            backend->mouseRelease(m_mouse);
        }
    }
}
