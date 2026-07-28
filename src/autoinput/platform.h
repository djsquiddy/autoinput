/**
 * @file platform.h
 * @author djsquiddy
 * @date April 2026
 */
#ifndef INCLUDE_AUTOINPUT_PLATFORM_H
#define INCLUDE_AUTOINPUT_PLATFORM_H
#pragma once

namespace autoinput
{
    struct Key;

    namespace platform
    {
        void signalEnd();
        void setupSignalHandler();
        int32_t getVirtualKey(const Key& key);
    }
}

#endif // INCLUDE_AUTOINPUT_PLATFORM_H
