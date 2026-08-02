/**
 * @file keyInfo.h
 * @author djsquiddy
 * @date July 2026
 */

#ifndef INCLUDE_AUTOINPUT_KEYINFO_H
#define INCLUDE_AUTOINPUT_KEYINFO_H
#pragma once

#include <cstdint>

#include "autoinput/types.h"

namespace autoinput
{
    struct KeyInfo
    {
        int32_t keyCode{ INVALID_KEY };
        int32_t functionKey{ INVALID_KEY };
        int32_t virtualKey{ 0 };
        MouseButton triggerButton{ MouseButton::None };
        Key triggerKey{};
        Mouse mouse{};
        Key key{};
        ActionState action{ ActionState::CLICK };
        bool isStartKey{ false };
        std::string name;
        std::string exclusiveGroup;
    };
}

#endif // INCLUDE_AUTOINPUT_KEYINFO_H
