/**
 * @file backendContext.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_BACKEND_CONTEXT_H
#define INCLUDE_AUTOINPUT_BACKEND_CONTEXT_H
#pragma once

#include <memory>
#include "autoinput/backend.h"

namespace autoinput
{
    /**
     * @brief Provides access to the platform-specific backend.
     */
    class BackendRegistry
    {
    public:
        /**
         * @brief Sets the global backend instance.
         * @param backend The backend to set.
         */
        static void setBackend(std::unique_ptr<IPlatformBackend> backend);

        /**
         * @brief Gets the current backend instance.
         * @return Pointer to the current backend, or nullptr if none set.
         */
        static IPlatformBackend* getBackend();

        /**
         * @brief Clears the current backend instance.
         */
        static void clearBackend();
    };

    /**
     * @brief Helper to temporarily override the backend, primarily for testing.
     */
    class ScopedBackendOverride
    {
    public:
        explicit ScopedBackendOverride(std::unique_ptr<IPlatformBackend> newBackend);
        ~ScopedBackendOverride();

        ScopedBackendOverride(const ScopedBackendOverride&) = delete;
        ScopedBackendOverride& operator=(const ScopedBackendOverride&) = delete;

    private:
        std::unique_ptr<IPlatformBackend> m_oldBackend;
    };
}

#endif // INCLUDE_AUTOINPUT_BACKEND_CONTEXT_H
