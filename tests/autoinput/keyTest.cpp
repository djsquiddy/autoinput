/**
 * @file keyTest.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include <gtest/gtest.h>

#include "autoinput/support/types.h"
#include "autoinput/input/keyboard.h"

namespace autoinput
{
    TEST(KeyToStringTest, FormatsSingleCharacter)
    {
        const Key key{
            .character = "a",
            .modifier = KeyModifier::None
        };

        // Verify string representation of unmodified character
        EXPECT_EQ(key.toString(), "a");
    }

    TEST(KeyToStringTest, FormatsSingleModifierWithCharacter)
    {
        const Key key{
            .character = "a",
            .modifier = KeyModifier::Ctrl
        };

        // Verify string representation with single Ctrl modifier
        EXPECT_EQ(key.toString(), "ctrl+a");
    }

    TEST(KeyToStringTest, FormatsMultipleModifiersWithCharacter)
    {
        const Key key{
            .character = "x",
            .modifier = KeyModifier::Ctrl | KeyModifier::Alt | KeyModifier::Shift
        };

        // Verify string representation with multiple combined modifiers
        EXPECT_EQ(key.toString(), "ctrl+alt+shift+x");
    }

    TEST(KeyToStringTest, FormatsFunctionKey)
    {
        const Key key{
            .character = "12",
            .modifier = KeyModifier::Function
        };

        // Verify string representation of function key (e.g. f12)
        EXPECT_EQ(key.toString(), "f12");
    }

    TEST(KeyToStringTest, FormatsModifiedFunctionKey)
    {
        const Key key{
            .character = "4",
            .modifier = KeyModifier::Ctrl | KeyModifier::Alt | KeyModifier::Function
        };

        // Verify string representation of modified function key (e.g. ctrl+alt+f4)
        EXPECT_EQ(key.toString(), "ctrl+alt+f4");
    }

    TEST(KeyModifierToStringTest, FormatsNoModifier)
    {
        // Verify KeyModifier::None produces empty string
        EXPECT_EQ(toString(KeyModifier::None), "");
    }

    TEST(KeyModifierToStringTest, FormatsSingleModifiers)
    {
        // Verify individual key modifier string conversions
        EXPECT_EQ(toString(KeyModifier::Ctrl), "ctrl");
        EXPECT_EQ(toString(KeyModifier::Alt), "alt");
        EXPECT_EQ(toString(KeyModifier::Shift), "shift");
        EXPECT_EQ(toString(KeyModifier::Meta), "meta");
    }

    TEST(KeyModifierToStringTest, FormatsCombinedModifiers)
    {
        // Verify combined key modifiers string conversions
        EXPECT_EQ(toString(KeyModifier::Ctrl | KeyModifier::Alt), "ctrl+alt");
        EXPECT_EQ(toString(KeyModifier::Ctrl | KeyModifier::Shift | KeyModifier::Meta), "ctrl+shift+meta");
    }

    TEST(KeyModifierToStringTest, FormatsFunctionModifier)
    {
        // Verify function key modifier string conversions
        EXPECT_EQ(toString(KeyModifier::Function), "f");
        EXPECT_EQ(toString(KeyModifier::Ctrl | KeyModifier::Function), "ctrl+f");
    }

    TEST(KeyFromStringTest, ParsesSingleCharacter)
    {
        const Key key = Key::fromString("a");

        // Verify parsed character and modifier for simple single character
        EXPECT_EQ(key.character, "a");
        EXPECT_EQ(key.modifier, KeyModifier::None);
    }

    TEST(KeyFromStringTest, ParsesCharacterCaseInsensitively)
    {
        const Key key = Key::fromString("A");

        // Verify uppercase character is normalized to lowercase
        EXPECT_EQ(key.character, "a");
        EXPECT_EQ(key.modifier, KeyModifier::None);
    }

    TEST(KeyFromStringTest, ParsesSingleModifierWithCharacter)
    {
        const Key key = Key::fromString("ctrl+a");

        // Verify parsed key character and Ctrl modifier flag
        EXPECT_EQ(key.character, "a");
        EXPECT_TRUE(static_cast<bool>(key.modifier & KeyModifier::Ctrl));
        // Verify other modifier flags are not set
        EXPECT_FALSE(static_cast<bool>(key.modifier & KeyModifier::Shift));
        EXPECT_FALSE(static_cast<bool>(key.modifier & KeyModifier::Alt));
        EXPECT_FALSE(static_cast<bool>(key.modifier & KeyModifier::Meta));
        EXPECT_FALSE(static_cast<bool>(key.modifier & KeyModifier::Function));
    }

    TEST(KeyFromStringTest, ParsesMultipleModifiersWithCharacter)
    {
        const Key key = Key::fromString("ctrl+shift+alt+meta+x");

        // Verify parsed key character and all combined modifier flags
        EXPECT_EQ(key.character, "x");
        EXPECT_TRUE(static_cast<bool>(key.modifier & KeyModifier::Ctrl));
        EXPECT_TRUE(static_cast<bool>(key.modifier & KeyModifier::Shift));
        EXPECT_TRUE(static_cast<bool>(key.modifier & KeyModifier::Alt));
        EXPECT_TRUE(static_cast<bool>(key.modifier & KeyModifier::Meta));
        // Verify function flag is not set for non-function key
        EXPECT_FALSE(static_cast<bool>(key.modifier & KeyModifier::Function));
    }

    TEST(KeyFromStringTest, ParsesModifiersCaseInsensitively)
    {
        const Key key = Key::fromString("CTRL+SHIFT+Z");

        // Verify case-insensitive modifier parsing and lowercasing of character
        EXPECT_EQ(key.character, "z");
        EXPECT_TRUE(static_cast<bool>(key.modifier & KeyModifier::Ctrl));
        EXPECT_TRUE(static_cast<bool>(key.modifier & KeyModifier::Shift));
    }

    TEST(KeyFromStringTest, ParsesFunctionKey)
    {
        const Key key = Key::fromString("f12");

        // Verify parsed function key number and function modifier flag
        EXPECT_EQ(key.character, "12");
        EXPECT_TRUE(static_cast<bool>(key.modifier & KeyModifier::Function));
        // Verify non-function modifier flags are not set
        EXPECT_FALSE(static_cast<bool>(key.modifier & KeyModifier::Ctrl));
        EXPECT_FALSE(static_cast<bool>(key.modifier & KeyModifier::Shift));
        EXPECT_FALSE(static_cast<bool>(key.modifier & KeyModifier::Alt));
        EXPECT_FALSE(static_cast<bool>(key.modifier & KeyModifier::Meta));
    }

    TEST(KeyFromStringTest, ParsesModifiedFunctionKey)
    {
        const Key key = Key::fromString("ctrl+alt+f4");

        // Verify parsed function key number and combined modifier flags
        EXPECT_EQ(key.character, "4");
        EXPECT_TRUE(static_cast<bool>(key.modifier & KeyModifier::Ctrl));
        EXPECT_TRUE(static_cast<bool>(key.modifier & KeyModifier::Alt));
        EXPECT_TRUE(static_cast<bool>(key.modifier & KeyModifier::Function));
        // Verify unspecified modifier flags are not set
        EXPECT_FALSE(static_cast<bool>(key.modifier & KeyModifier::Shift));
        EXPECT_FALSE(static_cast<bool>(key.modifier & KeyModifier::Meta));
    }

    TEST(KeyFromStringTest, ParsesSpecialKeys)
    {
        // Verify special key name parsing
        EXPECT_EQ(Key::fromString("end").character, "end");
        EXPECT_EQ(Key::fromString("esc").character, "esc");
        EXPECT_EQ(Key::fromString("space").character, "space");
    }

    TEST(KeyHandlerTest, StoresKey)
    {
        const Key key{
            .character = "a",
            .modifier = KeyModifier::Ctrl
        };

        const KeyHandler handler{ key };

        // Verify KeyHandler stores and retrieves original key structure and string representation
        EXPECT_EQ(handler.getKey(), key);
        EXPECT_EQ(handler.getKeyValue(), "ctrl+a");
    }

    TEST(KeyHandlerTest, UpdatesActiveState)
    {
        KeyHandler handler{
            Key{
                .character = "x",
                .modifier = KeyModifier::None
            }
        };

        // Verify KeyHandler is initially inactive
        EXPECT_FALSE(handler.getActive());

        handler.setActive(true);
        // Ensure KeyHandler active state updates to true
        EXPECT_TRUE(handler.getActive());

        handler.setActive(false);
        // Ensure KeyHandler active state updates to false
        EXPECT_FALSE(handler.getActive());
    }

    TEST(KeyHandlerTest, CopiesKeyAndActiveState)
    {
        KeyHandler original{
            Key{
                .character = "f4",
                .modifier = KeyModifier::Alt
            }
        };
        original.setActive(true);

        const KeyHandler copy{ original };

        // Verify copied KeyHandler preserves key, active state, and equality
        EXPECT_EQ(copy.getKey(), original.getKey());
        EXPECT_TRUE(copy.getActive());
        EXPECT_EQ(copy, original);
    }

    TEST(KeyHandlerTest, MoveConstructsKeyAndActiveState)
    {
        const Key key{
            .character = "z",
            .modifier = KeyModifier::Shift
        };

        KeyHandler original{ key };
        original.setActive(true);

        const KeyHandler moved{ std::move(original) };

        // Verify move-constructed KeyHandler retains key and active state
        EXPECT_EQ(moved.getKey(), key);
        EXPECT_TRUE(moved.getActive());
    }
}