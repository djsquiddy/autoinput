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
        const MouseHandler handler{ MouseButton::LEFT };

        EXPECT_EQ(handler.getMouseButton(), MouseButton::LEFT);
        EXPECT_EQ(handler.getButtonName(), "left");
    }

    TEST(MouseHandlerTest, UpdatesActiveState)
    {
        MouseHandler handler{ MouseButton::RIGHT };

        EXPECT_FALSE(handler.getActive());

        handler.setActive(true);
        EXPECT_TRUE(handler.getActive());

        handler.setActive(false);
        EXPECT_FALSE(handler.getActive());
    }

    TEST(MouseHandlerTest, CopiesButtonAndActiveState)
    {
        MouseHandler original{ MouseButton::MIDDLE };
        original.setActive(true);

        const MouseHandler copy{ original };

        EXPECT_EQ(copy.getMouseButton(), MouseButton::MIDDLE);
        EXPECT_TRUE(copy.getActive());
        EXPECT_EQ(copy, original);
    }

    TEST(MouseHandlerTest, MoveConstructsButtonAndActiveState)
    {
        MouseHandler original{ MouseButton::RIGHT };
        original.setActive(true);

        const MouseHandler moved{ std::move(original) };

        EXPECT_EQ(moved.getMouseButton(), MouseButton::RIGHT);
        EXPECT_TRUE(moved.getActive());
    }
    TEST(MouseButtonFromArgumentsTest, ParsesLeftAliases)
    {
        EXPECT_EQ(mouseButtonFromArguments("l"), MouseButton::LEFT);
        EXPECT_EQ(mouseButtonFromArguments("left"), MouseButton::LEFT);
    }

    TEST(MouseButtonFromArgumentsTest, ParsesMiddleAliases)
    {
        EXPECT_EQ(mouseButtonFromArguments("m"), MouseButton::MIDDLE);
        EXPECT_EQ(mouseButtonFromArguments("middle"), MouseButton::MIDDLE);
    }

    TEST(MouseButtonFromArgumentsTest, ParsesRightAliases)
    {
        EXPECT_EQ(mouseButtonFromArguments("r"), MouseButton::RIGHT);
        EXPECT_EQ(mouseButtonFromArguments("right"), MouseButton::RIGHT);
    }

    TEST(MouseButtonFromArgumentsTest, ParsesBackAndForward)
    {
        EXPECT_EQ(mouseButtonFromArguments("back"), MouseButton::BACK);
        EXPECT_EQ(mouseButtonFromArguments("forward"), MouseButton::FORWARD);
    }

    TEST(MouseButtonFromArgumentsTest, ReturnsNoneForUnknownInput)
    {
        EXPECT_EQ(mouseButtonFromArguments(""), MouseButton::NONE);
        EXPECT_EQ(mouseButtonFromArguments("unknown"), MouseButton::NONE);
    }

    TEST(MouseButtonToStringTest, FormatsMouseButtons)
    {
        EXPECT_EQ(mouseButtonToString(MouseButton::LEFT), "left");
        EXPECT_EQ(mouseButtonToString(MouseButton::MIDDLE), "middle");
        EXPECT_EQ(mouseButtonToString(MouseButton::RIGHT), "right");
        EXPECT_EQ(mouseButtonToString(MouseButton::BACK), "back");
        EXPECT_EQ(mouseButtonToString(MouseButton::FORWARD), "forward");
        EXPECT_EQ(mouseButtonToString(MouseButton::NONE), "");
    }

}