#include <gtest/gtest.h>
#include "autoinput/keyboard.h"
#include "autoinput/mouse.h"
#include "autoinput/linux/internal_data.h"

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
    // Actually, since we are on Windows, we are using the mock X11KeyboardData from internal_data.h.
    // And autoinput_linux.cpp's implementation of isX11KeyDown is in autoinput_x11.cpp.
    
    // In our case, the tests link with all project sources.
    // If we are on Windows, autoinput_x11.cpp is NOT linked.
    // Wait, let's look at CMakeLists.txt again.
}
