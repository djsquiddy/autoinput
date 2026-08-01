/**
 * @file backendContext.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/backendContext.h"
#include "autoinput/backend.h"

namespace autoinput
{
    void BackendRegistry::setBackend(std::unique_ptr<IPlatformBackend> backend)
    {
        g_backend = std::move(backend);
    }

    IPlatformBackend* BackendRegistry::getBackend()
    {
        return g_backend.get();
    }

    void BackendRegistry::clearBackend()
    {
        g_backend.reset();
    }

    ScopedBackendOverride::ScopedBackendOverride(std::unique_ptr<IPlatformBackend> newBackend)
    {
        m_oldBackend = std::move(g_backend);
        g_backend = std::move(newBackend);
    }

    ScopedBackendOverride::~ScopedBackendOverride()
    {
        g_backend = std::move(m_oldBackend);
    }
}
