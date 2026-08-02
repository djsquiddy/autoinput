/**
 * @file mouseTest.cpp
 * @author djsquiddy
 * @date July 2026
 */

#include <gtest/gtest.h>

#include "autoinput/types.h"
#include "autoinput/mouse.h"

namespace autoinput
{
    TEST(MouseHandlerTest, StoresMouseButton)
    {
        const MouseHandler handler{ MouseButton::Left };

        EXPECT_EQ(handler.getMouseButton(), MouseButton::Left);
        EXPECT_EQ(handler.getButtonName(), "left");
    }

    TEST(MouseHandlerTest, UpdatesActiveState)
    {
        MouseHandler handler{ MouseButton::Right };

        EXPECT_FALSE(handler.getActive());

        handler.setActive(true);
        EXPECT_TRUE(handler.getActive());

        handler.setActive(false);
        EXPECT_FALSE(handler.getActive());
    }

    TEST(MouseHandlerTest, CopiesButtonAndActiveState)
    {
        MouseHandler original{ MouseButton::Middle };
        original.setActive(true);

        const MouseHandler copy{ original };

        EXPECT_EQ(copy.getMouseButton(), MouseButton::Middle);
        EXPECT_TRUE(copy.getActive());
        EXPECT_EQ(copy, original);
    }

    TEST(MouseHandlerTest, MoveConstructsButtonAndActiveState)
    {
        MouseHandler original{ MouseButton::Right };
        original.setActive(true);

        const MouseHandler moved{ std::move(original) };

        EXPECT_EQ(moved.getMouseButton(), MouseButton::Right);
        EXPECT_TRUE(moved.getActive());
    }
    TEST(MouseButtonFromArgumentsTest, ParsesLeftAliases)
    {
        EXPECT_EQ(mouseButtonFromArguments("l"), MouseButton::Left);
        EXPECT_EQ(mouseButtonFromArguments("left"), MouseButton::Left);
    }

    TEST(MouseButtonFromArgumentsTest, ParsesMiddleAliases)
    {
        EXPECT_EQ(mouseButtonFromArguments("m"), MouseButton::Middle);
        EXPECT_EQ(mouseButtonFromArguments("middle"), MouseButton::Middle);
    }

    TEST(MouseButtonFromArgumentsTest, ParsesRightAliases)
    {
        EXPECT_EQ(mouseButtonFromArguments("r"), MouseButton::Right);
        EXPECT_EQ(mouseButtonFromArguments("right"), MouseButton::Right);
    }

    TEST(MouseButtonFromArgumentsTest, ParsesBackAndForward)
    {
        EXPECT_EQ(mouseButtonFromArguments("back"), MouseButton::Back);
        EXPECT_EQ(mouseButtonFromArguments("forward"), MouseButton::Forward);
    }

    TEST(MouseButtonFromArgumentsTest, ReturnsNoneForUnknownInput)
    {
        EXPECT_EQ(mouseButtonFromArguments(""), MouseButton::None);
        EXPECT_EQ(mouseButtonFromArguments("unknown"), MouseButton::None);
    }

    TEST(MouseButtonToStringTest, FormatsMouseButtons)
    {
        EXPECT_EQ(mouseButtonToString(MouseButton::Left), "left");
        EXPECT_EQ(mouseButtonToString(MouseButton::Middle), "middle");
        EXPECT_EQ(mouseButtonToString(MouseButton::Right), "right");
        EXPECT_EQ(mouseButtonToString(MouseButton::Back), "back");
        EXPECT_EQ(mouseButtonToString(MouseButton::Forward), "forward");
        EXPECT_EQ(mouseButtonToString(MouseButton::None), "");
    }

}