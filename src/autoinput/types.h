/**
 * @file types.h
 * @author djsquiddy
 * @date March 2026
 */
#ifndef INCLUDE_AUTOINPUT_TYPES_H
#define INCLUDE_AUTOINPUT_TYPES_H
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <any>
#include <type_traits>
#include <functional>
#include <iostream>
#include <format>

// NOLINTBEGIN(bugprone-macro-parentheses)
#define AUTOINPUT_ENABLE_ENUM_BITWISE_OPERATORS(enum_class) \
    constexpr enum_class operator|(enum_class lhs, enum_class rhs) \
    { \
        using enum_class_t = std::underlying_type_t<enum_class>; \
        return static_cast<enum_class>(static_cast<enum_class_t>(lhs) | static_cast<enum_class_t>(rhs)); \
    } \
    constexpr enum_class operator|=(enum_class& lhs, enum_class rhs) \
    { \
        lhs = lhs | rhs; \
        return lhs; \
    } \
    constexpr enum_class operator&(enum_class lhs, enum_class rhs) \
    { \
        using enum_class_t = std::underlying_type_t<enum_class>; \
        return static_cast<enum_class>(static_cast<enum_class_t>(lhs) & static_cast<enum_class_t>(rhs)); \
    } \
    constexpr enum_class operator&=(enum_class& lhs, enum_class rhs) \
    { \
        lhs = lhs & rhs; \
        return lhs; \
    } \
    constexpr enum_class operator^(enum_class lhs, enum_class rhs) \
    { \
        using enum_class_t = std::underlying_type_t<enum_class>; \
        return static_cast<enum_class>(static_cast<enum_class_t>(lhs) ^ static_cast<enum_class_t>(rhs)); \
    } \
    constexpr enum_class operator^=(enum_class& lhs, enum_class rhs) \
    { \
        lhs = lhs ^ rhs; \
        return lhs; \
    } \
    constexpr enum_class operator~(enum_class lhs) \
    { \
        using enum_class_t = std::underlying_type_t<enum_class>; \
        return static_cast<enum_class>(~static_cast<enum_class_t>(lhs)); \
    } \


// NOLINTEND(bugprone-macro-parentheses)

namespace autoinput
{
    using u8 = uint8_t;
    using u16 = uint16_t;
    using u32 = uint32_t;
    using u64 = uint64_t;

    using i8 = int8_t;
    using i16 = int16_t;
    using i32 = int32_t;
    using i64 = int64_t;

    using f32 = float;
    using f64 = double;

    struct Point
    {
        i32 x{ 0 };
        i32 y{ 0 };
    };

    struct KeyboardData
    {
        std::any internal;
    };

    struct MouseData
    {
        std::any internal;
    };
    template<typename T>
    struct HashFunction {};


    struct Quoted
    {
        Quoted(const std::string_view str) : m_sv(str) {}
        std::string_view m_sv;
    };

    enum class ActionState
    {
        INVALID = 0,
        CLICK = 1,
        HOLD = 2,
    };

    enum class StatusNotificationMode
    {
        Off = 0,
        Console = 1 << 0,
        Desktop = 1 << 1,
        Both = Console | Desktop
    };
    AUTOINPUT_ENABLE_ENUM_BITWISE_OPERATORS(StatusNotificationMode);

    StatusNotificationMode statusNotificationModeFromString(std::string_view str);
    std::string_view statusNotificationModeToString(StatusNotificationMode mode);

    enum class RecordedEventType
    {
        Invalid = 0,
        KeyDown,
        KeyUp,
        MouseDown,
        MouseUp,
        MouseMove
    };

    std::string_view recordedEventTypeToString(RecordedEventType type);
    RecordedEventType recordedEventTypeFromString(std::string_view str);

    enum class MouseButton : uint8_t
    {
        None = 0,
        Left = 1 << 0,
        Middle = 1 << 1,
        Right = 1 << 2,
        Back = 1 << 3,
        Forward = 1 << 4,
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

    enum class KeyModifier : uint8_t
    {
        None = 0,
        Ctrl = 1 << 0,
        Shift = 1 << 2,
        Alt = 1 << 3,
        Meta = 1 << 4, // Windows key
        Function = 1 << 5,
    };

    std::string toString(KeyModifier modifier);
    KeyModifier keyModifierFromStringVector(const std::vector<std::string>& keyValue);
    template<>
    struct HashFunction<KeyModifier>
    {
        size_t operator()(const KeyModifier& button) const
        {
            using key_t = std::underlying_type_t<KeyModifier>;
            return std::hash<key_t>()(static_cast<key_t>(button));
        }
    };
    AUTOINPUT_ENABLE_ENUM_BITWISE_OPERATORS(KeyModifier);

    struct Mouse
    {
        Mouse() = default;
        // ReSharper disable once CppNonExplicitConvertingConstructor
        Mouse(const MouseButton b) : button(b) {}
        Mouse(const MouseButton b, const KeyModifier m) : button(b), modifier(m) {}

        MouseButton button{ MouseButton::None };
        KeyModifier modifier{ KeyModifier::None };

        [[nodiscard]] static Mouse fromString(const std::string_view& keyValue);
        [[nodiscard]] std::string toString() const;
        bool operator==(const Mouse& rhs) const
        {
            return this->button == rhs.button && this->modifier == rhs.modifier;
        }
    };

    template<>
    struct HashFunction<Mouse>
    {
        size_t operator()(const Mouse& mouse) const
        {
            using button_t = std::underlying_type_t<MouseButton>;
            return std::hash<button_t>()(static_cast<button_t>(mouse.button)) ^ std::hash<KeyModifier>()(mouse.modifier);
        }
    };

    constexpr int32_t INVALID_KEY = -1;

    struct KeyState
    {
        int32_t keyCode{ INVALID_KEY };
        int32_t functionKey{ INVALID_KEY };
        int32_t virtualKey{ 0 };
        KeyModifier modifier{ KeyModifier::None };
    };

    struct Key
    {
        std::string character{};
        KeyModifier modifier{ KeyModifier::None };

        [[nodiscard]] static Key fromString(const std::string_view& keyValue);
        [[nodiscard]] static Key fromKeyState(const KeyState& state);
        [[nodiscard]] std::string toString() const;
        bool operator==(const Key& rhs) const
        {
            return this->character == rhs.character && this->modifier == rhs.modifier;
        }
    };

    template<>
    struct HashFunction<Key>
    {
        size_t operator()(const Key& handler) const
        {
            return std::hash<std::string>()(handler.character) ^ std::hash<KeyModifier>()(handler.modifier);
        }
    };

    ActionState actionStateFromArguments(std::string_view actionType);

    MouseButton mouseButtonFromArguments(std::string_view button);
    std::string mouseButtonToString(const MouseButton& mouseButton);
    int32_t parseStringToInt(std::string_view value);
    std::string actionStateToString(ActionState actionState);
}

// ReSharper disable CppMemberFunctionMayBeStatic
// NOLINTBEGIN(*-convert-member-functions-to-static)

// Specialize std::formatter for MouseButton
template <>
struct std::formatter<autoinput::MouseButton>
{
    constexpr auto parse(const std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    // Format the value
    auto format(const autoinput::MouseButton mouseButton, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}", autoinput::mouseButtonToString(mouseButton));
    }
};

template<>
struct std::formatter<autoinput::Mouse>
{
    constexpr auto parse(const std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const autoinput::Mouse mouse, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}", mouse.toString());
    }
};

template<>
struct std::formatter<autoinput::Key>
{
    constexpr auto parse(const std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const autoinput::Key& key, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}", key.toString());
    }
};


// copied from https://accu.org/journals/overload/32/182/collyer/
template<>
struct std::formatter<autoinput::Quoted>
{
    constexpr auto parse(const format_parse_context& parse_ctx)
    {
        auto iter = parse_ctx.begin();
        auto get_char = [&]() -> int { return iter != parse_ctx.end() ? *iter : 0; };
        auto c = get_char();
        if (c == 0 || c == '}')
        {
            return iter;
        }
        m_quote = c;
        ++iter;
        if ((c = get_char()) != 0 && c != '}')
        {
            m_esc = c;
            ++iter;
        }
        if ((c = get_char()) != 0 && c != '}')
        {
            throw format_error(
              "Invalid Quoted format specification");
        }
        return iter;
    }
    auto format(const autoinput::Quoted& p, format_context& format_ctx) const
    {
        string_view::size_type pos = 0;
        const string_view::size_type end = p.m_sv.length();
        auto out = format_ctx.out();
        *out++ = m_quote;
        while (pos < end)
        {
            const auto c = p.m_sv[pos++];
            if (c == m_quote || c == m_esc)
            {
                *out++ = m_esc;
            }
            *out++ = c; // 11
        }
        *out++ = m_quote;   // 12
        return out; // 13
    }
private:
    char m_quote{ '"' };
    char m_esc{ '\\' };
};
// NOLINTEND(*-convert-member-functions-to-static)
// ReSharper restore CppMemberFunctionMayBeStatic

#endif // INCLUDE_AUTOINPUT_TYPES_H
