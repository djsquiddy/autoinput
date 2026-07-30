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
        if (m_isPressed || !g_backend) return;
        g_backend->keyPress(m_key);
        m_isPressed = true;
    }

    void KeyHandler::release()
    {
        if (!m_isPressed || !g_backend) return;
        g_backend->keyRelease(m_key);
        m_isPressed = false;
    }

    void MouseHandler::press()
    {
        if (m_isPressed || !g_backend) return;
        g_backend->mousePress(m_mouse);
        m_isPressed = true;
    }

    void MouseHandler::release()
    {
        if (!m_isPressed || !g_backend) return;
        g_backend->mouseRelease(m_mouse);
        m_isPressed = false;
    }
}
