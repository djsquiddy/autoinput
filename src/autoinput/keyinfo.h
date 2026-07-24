/**
 * @file keyinfo.h
 * @author djsquiddy
 * @date July 2026
 */

#ifndef INCLUDE_AUTOINPUT_KEYINFO_H
#define INCLUDE_AUTOINPUT_KEYINFO_H
#pragma once

namespace autoinput
{
    struct KeyInfo
    {
        int32_t keyCode{ INVALID_KEY };
        int32_t functionKey{ INVALID_KEY };
        MouseButton mouseButton{ MouseButton::NONE };
        bool isStartKey{ false };
    };
}

#endif // INCLUDE_AUTOINPUT_KEYINFO_H
