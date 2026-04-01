//
// Created by djsquiddy on 3/9/2026.
//

#ifndef INCLUDE_AUTOINPUT_KEYBOARD_H
#define INCLUDE_AUTOINPUT_KEYBOARD_H
#pragma once

namespace autoinput
{
    struct KeyboardData;

    struct KeyboardInput
    {
        explicit KeyboardInput(KeyboardData& data);
        KeyboardData& data;

        [[nodiscard]] bool isKeyDown() const;
        [[nodiscard]] bool isKeyUp() const;
        [[nodiscard]] bool isSysKey() const;
        [[nodiscard]] int32_t getChar() const;
        [[nodiscard]] int32_t functionKey() const;

        void printInfo() const;
    };
}
#endif // INCLUDE_AUTOINPUT_KEYBOARD_H