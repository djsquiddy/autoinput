/**
 * @file backendFactory.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_BACKEND_FACTORY_H
#define INCLUDE_AUTOINPUT_BACKEND_FACTORY_H
#pragma once

#include "autoinput/backend.h"
#include <memory>

namespace autoinput
{
    /**
     * @brief Factory for creating the appropriate platform backend.
     */
    class BackendFactory
    {
    public:
        /**
         * @brief Creates a platform backend based on the current system and environment.
         * @return A unique pointer to the created backend, or nullptr if none could be created.
         */
        static std::unique_ptr<IPlatformBackend> createPlatformBackend();

        /**
         * @brief Internal helper to detect the appropriate Linux backend.
         * Exposed for testing purposes.
         * @return A unique pointer to the detected Linux backend.
         */
        static std::unique_ptr<IPlatformBackend> detectLinuxBackend();
    };
}

#endif // INCLUDE_AUTOINPUT_BACKEND_FACTORY_H
