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
#include <format>

#define AUTOINPUT_UNUSED(...) static_cast<void>(__VA_ARGS__)

// Enable operators only for specific enums (manual or via trait)
#define AUTOINPUT_ENABLE_ENUM_BITWISE_OPERATORS(E) template<> inline constexpr bool is_flag_enum<E> = true;

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

    template<typename T>
    inline constexpr bool is_flag_enum = false;

    // Concept checking the trait
    template<typename T>
    concept FlagEnum = std::is_enum_v<T> && is_flag_enum<T>;

    // Operator enabled ONLY if FlagEnum concept is satisfied
    template<FlagEnum E>
    constexpr E operator|(E lhs, E rhs)
    {
        using U = std::underlying_type_t<E>;
        return static_cast<E>(static_cast<U>(lhs) | static_cast<U>(rhs));
    }

    template<FlagEnum E>
    constexpr E operator|=(E& lhs, E rhs)
    {
        lhs = lhs | rhs;
        return lhs;
    }

    template<FlagEnum E>
    constexpr E operator&(E lhs, E rhs)
    {
        using U = std::underlying_type_t<E>;
        return static_cast<E>(static_cast<U>(lhs) & static_cast<U>(rhs));
    }

    template<FlagEnum E>
    constexpr E operator&=(E& lhs, E rhs)
    {
        lhs = lhs & rhs;
        return lhs;
    }

    template<FlagEnum E>
    constexpr E operator^(E lhs, E rhs)
    {
        using U = std::underlying_type_t<E>;
        return static_cast<E>(static_cast<U>(lhs) ^ static_cast<U>(rhs));
    }

    template<FlagEnum E>
    constexpr E operator^=(E& lhs, E rhs)
    {
        lhs = lhs ^ rhs;
        return lhs;
    }

    template<FlagEnum E>
    constexpr E operator~(E lhs)
    {
        using U = std::underlying_type_t<E>;
        return static_cast<E>(~static_cast<U>(lhs));
    }

    template<FlagEnum E>
    bool isFlagSet(const E lhs, const E rhs)
    {
        return (lhs & rhs) == rhs;
    }

    template<FlagEnum E>
    void setFlag(E& lhs, const E rhs)
    {
        lhs |= rhs;
    }

    template<FlagEnum E>
    void clearFlag(E& lhs, const E rhs)
    {
        lhs &= ~rhs;
    }

    struct Point
    {
        i32 x{ 0 };
        i32 y{ 0 };
    };

    struct AppWindowInfo
    {
        std::string processName;
        std::string windowTitle;
        u64 pid{ 0 };
        std::string executablePath;
        std::string backendId;
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

    [[nodiscard]] std::vector<std::string_view> getAllStatusNotificationModes();
    [[nodiscard]] StatusNotificationMode statusNotificationModeFromString(std::string_view str);
    [[nodiscard]] std::string_view statusNotificationModeToString(StatusNotificationMode mode);

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
    AUTOINPUT_ENABLE_ENUM_BITWISE_OPERATORS(MouseButton);

    template<>
    struct HashFunction<MouseButton>
    {
        size_t operator()(const MouseButton& button) const
        {
            using button_t = std::underlying_type_t<MouseButton>;
            return std::hash<button_t>()(static_cast<button_t>(button));
        }
    };

    enum class KeyModifier : uint8_t
    {
        None = 0,
        Ctrl = 1 << 0,
        Shift = 1 << 2,
        Alt = 1 << 3,
        Meta = 1 << 4, // Windows key
        Function = 1 << 5,
    };

    AUTOINPUT_ENABLE_ENUM_BITWISE_OPERATORS(KeyModifier);

    /**
     * @brief Converts a KeyModifier to its string representation.
     * @param modifier The key modifier.
     * @return The string representation.
     */
    std::string toString(KeyModifier modifier);

    /**
     * @brief Parses a vector of strings into a KeyModifier bitmask.
     * @param keyValue A vector of modifier names (e.g. ["ctrl", "shift"]).
     * @return The combined KeyModifier mask.
     */
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

    struct Mouse
    {
        /**
         * @brief Default constructor.
         */
        Mouse() = default;

        /**
         * @brief Constructs a Mouse object from a button.
         * @param b The mouse button.
         */
        // ReSharper disable once CppNonExplicitConvertingConstructor
        Mouse(const MouseButton b) : button(b) {}

        /**
         * @brief Constructs a Mouse object from a button and modifier.
         * @param b The mouse button.
         * @param m The key modifier.
         */
        Mouse(const MouseButton b, const KeyModifier m) : button(b), modifier(m) {}

        MouseButton button{ MouseButton::None };
        KeyModifier modifier{ KeyModifier::None };

        /**
         * @brief Parses a string to a Mouse object.
         * @param keyValue The string representation (e.g. "ctrl+left").
         * @return The parsed Mouse object.
         */
        [[nodiscard]] static Mouse fromString(const std::string_view& keyValue);

        /**
         * @brief Converts the Mouse object to its string representation.
         * @return The string representation.
         */
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

        /**
         * @brief Parses a string to a Key object.
         * @param keyValue The string representation (e.g. "ctrl+a").
         * @return The parsed Key object.
         */
        [[nodiscard]] static Key fromString(const std::string_view& keyValue);

        /**
         * @brief Creates a Key object from a platform-specific KeyState.
         * @param state The key state.
         * @return The created Key object.
         */
        [[nodiscard]] static Key fromKeyState(const KeyState& state);

        /**
         * @brief Converts the Key object to its string representation.
         * @return The string representation.
         */
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

    /**
     * @brief Parses an action state string.
     * @param actionType The action type string (e.g. "click", "hold").
     * @return The corresponding ActionState.
     */
    ActionState actionStateFromArguments(std::string_view actionType);

    /**
     * @brief Parses a mouse button string.
     * @param button The button name (e.g. "left").
     * @return The corresponding MouseButton.
     */
    MouseButton mouseButtonFromArguments(std::string_view button);

    /**
     * @brief Converts a MouseButton to its string representation.
     * @param mouseButton The mouse button.
     * @return The string representation.
     */
    std::string mouseButtonToString(const MouseButton& mouseButton);

    /**
     * @brief Parses a string representing an integer.
     * @param value The string to parse.
     * @return The parsed integer, or -1 on failure.
     */
    int32_t parseStringToInt(std::string_view value);

    /**
     * @brief Converts an ActionState to its string representation.
     * @param actionState The action state.
     * @return The string representation.
     */
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
