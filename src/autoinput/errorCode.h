/**
 * @file errorCode.h
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_ERROR_CODE_H
#define INCLUDE_AUTOINPUT_ERROR_CODE_H
#pragma once

#include <cstdint>
#include <cstdlib>

namespace autoinput
{
    enum class ErrorCode : int32_t
    {
        Success = EXIT_SUCCESS,
        InvalidParam = 101,
        FailedToInstallHooks = 102,
        UnhandledException = 103,
        FailedToLoadConfig = 104
    };
}

#endif // INCLUDE_AUTOINPUT_ERROR_CODE_H
