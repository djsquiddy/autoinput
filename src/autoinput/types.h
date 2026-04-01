//
// Created by djsquiddy on 3/9/2026.
//

#ifndef INCLUDE_AUTOINPUT_TYPES_H
#define INCLUDE_AUTOINPUT_TYPES_H
#pragma once

#include <chrono>
#include <cstdint>
#include <iosfwd>
#include <string_view>

// NOLINTBEGIN(bugprone-macro-parentheses)
#define AUTOINPUT_ENABLE_ENUM_BITWISE_OPERATORS(enum_class) \
    constexpr enum_class operator|(enum_class lhs, enum_class rhs) \
    { \
        using state_t = std::underlying_type_t<enum_class>; \
        return static_cast<enum_class>(static_cast<state_t>(lhs) | static_cast<state_t>(rhs)); \
    } \
    constexpr enum_class operator|=(enum_class& lhs, enum_class rhs) \
    { \
        lhs = lhs | rhs; \
        return lhs; \
    } \
    constexpr enum_class operator&(enum_class lhs, enum_class rhs) \
    { \
        using state_t = std::underlying_type_t<enum_class>; \
        return static_cast<enum_class>(static_cast<state_t>(lhs) & static_cast<state_t>(rhs)); \
    } \
    constexpr enum_class operator&=(enum_class& lhs, enum_class rhs) \
    { \
        lhs = lhs & rhs; \
        return lhs; \
    } \
    constexpr enum_class operator^(enum_class lhs, enum_class rhs) \
    { \
        using state_t = std::underlying_type_t<enum_class>; \
        return static_cast<enum_class>(static_cast<state_t>(lhs) ^ static_cast<state_t>(rhs)); \
    } \
    constexpr enum_class operator^=(enum_class& lhs, enum_class rhs) \
    { \
        lhs = lhs ^ rhs; \
        return lhs; \
    } \
    constexpr enum_class operator~(enum_class lhs) \
    { \
        using state_t = std::underlying_type_t<enum_class>; \
        return static_cast<enum_class>(~static_cast<state_t>(lhs)); \
    } \


// NOLINTEND(bugprone-macro-parentheses)

namespace autoinput
{
    template<typename T>
    struct HashFunction {};

    // Originally from: https://stackoverflow.com/a/60344943
    class bold
    {
    public:
        explicit bold(const std::string_view& s);

        friend std::ostream& operator<<(std::ostream& os, const bold& b);
    private:
        std::string_view const &m_string;
    };

    enum class ButtonState
    {
        INVALID = 0,
        CLICK = 1,
        HOLD = 2,
    };

    enum class MouseButton : uint8_t
    {
        NONE = 0,
        LEFT = 1 << 0,
        MIDDLE = 1 << 1,
        RIGHT = 1 << 2,
        BACK = 1 << 3,
        FORWARD = 1 << 4,
    };

    template<>
    struct HashFunction<MouseButton>
    {
        size_t operator()(const MouseButton& button) const
        {
            using button_t = std::underlying_type_t<MouseButton>;
            return std::hash<button_t>()(static_cast<button_t>(button));
        }
    };
    AUTOINPUT_ENABLE_ENUM_BITWISE_OPERATORS(MouseButton);

    constexpr int32_t PRESS_FPS = 10;
    constexpr int32_t DEFAULT_DELAY = 1 / PRESS_FPS;

    constexpr int32_t INVALID_KEY = -1;

    ButtonState buttonStateFromArguments(std::string_view buttonType);
    MouseButton mouseButtonFromArguments(std::string_view button);
    std::string mouseButtonToString(const MouseButton& mouseButton);
    int32_t parseStringToInt(std::string_view value);
}

#endif // INCLUDE_AUTOINPUT_TYPES_H