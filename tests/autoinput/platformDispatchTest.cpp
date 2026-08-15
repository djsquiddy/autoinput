/**
 * @file platformDispatchTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include "autoinput/input/keyboard.h"
#include "autoinput/input/mouse.h"
#include "autoinput/platform/linux/internalDataLinux.h"

using namespace autoinput;

class PlatformDispatchTest : public ::testing::Test {};

TEST_F(PlatformDispatchTest, KeyboardInputDispatchX11)
{
    KeyboardData data;
    X11KeyboardData x11Data;
    x11Data.keycode = 65; // 'A' in some layout
    data.internal = x11Data;

    KeyboardInput input(data);
    
    // We can't easily check the result of isKeyDown because it calls isX11KeyDown which might not be mocked
    // but we CAN check that it doesn't crash and correctly identifies the internal type.
    // Actually, since we are on Windows, we are using the mock X11KeyboardData from internalDataLinux.h.
    // And autoInputLinux.cpp's implementation of isX11KeyDown is in autoInputX11.cpp.
    
    // In our case, the tests link with all project sources.
    // If we are on Windows, autoInputX11.cpp is NOT linked.
    // Wait, let's look at CMakeLists.txt again.
}
