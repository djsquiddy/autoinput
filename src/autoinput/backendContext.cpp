/**
 * @file backendContext.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/backendContext.h"
#include "autoinput/backend.h"
#include "autoinput/autoInput.h"

namespace autoinput
{
    void BackendRegistry::setBackend(std::unique_ptr<IPlatformBackend> backend)
    {
        if (g_program)
        {
            g_program->setBackend(std::move(backend));
        }
    }

    IPlatformBackend* BackendRegistry::getBackend()
    {
        return g_program ? g_program->getBackend() : nullptr;
    }

    void BackendRegistry::clearBackend()
    {
        if (g_program)
        {
            g_program->setBackend(nullptr);
        }
    }

    ScopedBackendOverride::ScopedBackendOverride(std::unique_ptr<IPlatformBackend> newBackend)
    {
        if (g_program)
        {
            m_oldBackend = g_program->releaseBackend();
            g_program->setBackend(std::move(newBackend));
        }
    }

    ScopedBackendOverride::~ScopedBackendOverride()
    {
        if (g_program)
        {
            g_program->setBackend(std::move(m_oldBackend));
        }
    }
}
