/**
 * @file mouseTest.cpp
 * @author djsquiddy
 * @date July 2026
 */

#include <gtest/gtest.h>

#include "autoinput/support/types.h"
#include "autoinput/input/mouse.h"

namespace autoinput
{
    TEST(MouseHandlerTest, StoresMouseButton)
    {
        const MouseHandler handler{ MouseButton::Left };

        // Verify that the handler stores and returns the initialized mouse button
        EXPECT_EQ(handler.getMouseButton(), MouseButton::Left);
        // Verify that getButtonName returns the matching lowercase button name string
        EXPECT_EQ(handler.getButtonName(), "left");
    }

    TEST(MouseHandlerTest, UpdatesActiveState)
    {
        MouseHandler handler{ MouseButton::Right };

        // Verify that the handler is inactive by default
        EXPECT_FALSE(handler.getActive());

        handler.setActive(true);
        // Verify that setActive(true) updates active state to true
        EXPECT_TRUE(handler.getActive());

        handler.setActive(false);
        // Verify that setActive(false) updates active state to false
        EXPECT_FALSE(handler.getActive());
    }

    TEST(MouseHandlerTest, CopiesButtonAndActiveState)
    {
        MouseHandler original{ MouseButton::Middle };
        original.setActive(true);

        const MouseHandler copy{ original };

        // Verify that copy construction preserves the mouse button
        EXPECT_EQ(copy.getMouseButton(), MouseButton::Middle);
        // Verify that copy construction preserves the active state
        EXPECT_TRUE(copy.getActive());
        // Verify that copy equals the original handler
        EXPECT_EQ(copy, original);
    }

    TEST(MouseHandlerTest, MoveConstructsButtonAndActiveState)
    {
        MouseHandler original{ MouseButton::Right };
        original.setActive(true);

        const MouseHandler moved{ std::move(original) };

        // Verify that move construction preserves the mouse button
        EXPECT_EQ(moved.getMouseButton(), MouseButton::Right);
        // Verify that move construction preserves the active state
        EXPECT_TRUE(moved.getActive());
    }
    TEST(MouseButtonFromArgumentsTest, ParsesLeftAliases)
    {
        // Verify that single-letter alias "l" and full name "left" parse to Left button
        EXPECT_EQ(mouseButtonFromArguments("l"), MouseButton::Left);
        EXPECT_EQ(mouseButtonFromArguments("left"), MouseButton::Left);
    }

    TEST(MouseButtonFromArgumentsTest, ParsesMiddleAliases)
    {
        // Verify that single-letter alias "m" and full name "middle" parse to Middle button
        EXPECT_EQ(mouseButtonFromArguments("m"), MouseButton::Middle);
        EXPECT_EQ(mouseButtonFromArguments("middle"), MouseButton::Middle);
    }

    TEST(MouseButtonFromArgumentsTest, ParsesRightAliases)
    {
        // Verify that single-letter alias "r" and full name "right" parse to Right button
        EXPECT_EQ(mouseButtonFromArguments("r"), MouseButton::Right);
        EXPECT_EQ(mouseButtonFromArguments("right"), MouseButton::Right);
    }

    TEST(MouseButtonFromArgumentsTest, ParsesBackAndForward)
    {
        // Verify that "back" and "forward" string arguments parse to Back and Forward buttons
        EXPECT_EQ(mouseButtonFromArguments("back"), MouseButton::Back);
        EXPECT_EQ(mouseButtonFromArguments("forward"), MouseButton::Forward);
    }

    TEST(MouseButtonFromArgumentsTest, ReturnsNoneForUnknownInput)
    {
        // Verify that empty or invalid button argument strings return MouseButton::None
        EXPECT_EQ(mouseButtonFromArguments(""), MouseButton::None);
        EXPECT_EQ(mouseButtonFromArguments("unknown"), MouseButton::None);
    }

    TEST(MouseButtonToStringTest, FormatsMouseButtons)
    {
        // Verify that each MouseButton enum value converts to the correct lowercase string representation
        EXPECT_EQ(mouseButtonToString(MouseButton::Left), "left");
        EXPECT_EQ(mouseButtonToString(MouseButton::Middle), "middle");
        EXPECT_EQ(mouseButtonToString(MouseButton::Right), "right");
        EXPECT_EQ(mouseButtonToString(MouseButton::Back), "back");
        EXPECT_EQ(mouseButtonToString(MouseButton::Forward), "forward");
        EXPECT_EQ(mouseButtonToString(MouseButton::None), "");
    }

}