/**
 * @file backendFactory.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_PLATFORM_BACKENDFACTORY_H
#define INCLUDE_AUTOINPUT_PLATFORM_BACKENDFACTORY_H
#pragma once

#include "autoinput/platform/backend.h"
#include "autoinput/platform/environment.h"
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
         * @brief Creates a platform backend using a specific environment.
         * @param environment The environment to use for detection.
         * @return A unique pointer to the created backend, or nullptr if none could be created.
         */
        static std::unique_ptr<IPlatformBackend> createPlatformBackend(const IEnvironment& environment);

    private:
        /**
         * @brief Internal helper to detect the appropriate Linux backend.
         * @param environment The environment to use for detection.
         * @return A unique pointer to the detected Linux backend.
         */
        static std::unique_ptr<IPlatformBackend> detectLinuxBackend(const IEnvironment& environment);
    };
}

#endif // INCLUDE_AUTOINPUT_PLATFORM_BACKENDFACTORY_H
