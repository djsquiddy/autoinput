/**
 * @file mouseTriggerLogicTest.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include <gtest/gtest.h>
#include "autoinput/autoInput.h"
#include "autoinput/arguments.h"

namespace autoinput
{
    TEST(ProgramInitTest, MapsMouseButtonsAsTriggers)
    {
        Program program;
        char* argv[] = {(char*)"autoinput", (char*)"left", (char*)"-s", (char*)"back", (char*)"-e", (char*)"forward"};
        int argc = sizeof(argv) / sizeof(char*);

        ASSERT_TRUE(program.arguments().parseArguments(gsl::make_span(argv, argc)));
        program.setBackend(std::make_unique<FakeBackend>());
        ASSERT_TRUE(program.init());

        const auto& keyInfo = program.getKeyInfo();
        // Expecting 2 KeyInfo: one for start (back) and one for end (forward)
        ASSERT_EQ(keyInfo.size(), 2);

        // Start key info
        EXPECT_EQ(keyInfo[0].triggerButton, MouseButton::Back);
        EXPECT_EQ(keyInfo[0].mouse.button, MouseButton::Left);
        EXPECT_TRUE(keyInfo[0].isStartKey);

        // End key info
        EXPECT_EQ(keyInfo[1].triggerButton, MouseButton::Forward);
        EXPECT_FALSE(keyInfo[1].isStartKey);
    }
}
