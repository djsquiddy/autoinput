/**
 * @file mouseTriggerLogicTest.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include <gtest/gtest.h>
#include "autoinput/autoInput.h"
#include "autoinput/cli/runCommand.h"
#include "autoinput/cli/commandBase.h"

namespace autoinput
{
    TEST(ProgramInitTest, RunCommandParsesTriggers)
    {
        cli::CommandContext context;
        cli::RunCommand command(context);
        std::vector<std::string> argvStr = { "run", "--button", "left", "--start", "back", "--end", "forward" };
        std::vector<char*> argv;
        for (const auto& s : argvStr) argv.push_back(const_cast<char*>(s.data()));

        i32 index = 1;
        ASSERT_TRUE(command.parse(gsl::make_span(argv.data(), argv.size()), index));
        ASSERT_TRUE(command.validate());
    }

    TEST(ProgramInitTest, MapsMouseButtonsAsTriggers)
    {
        Program program;
        ProgramArguments& arguments = program.arguments();
        arguments.buttons.push_back(Mouse(MouseButton::Left));
        arguments.targetActions.push_back(ActionState::CLICK);
        arguments.startKeys.push_back("back");
        arguments.endKey = "forward";
        ASSERT_TRUE(arguments.postParseArguments());

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
