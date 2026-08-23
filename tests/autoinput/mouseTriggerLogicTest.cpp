/**
 * @file mouseTriggerLogicTest.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include "autoinput/app/autoinput.h"
#include "autoinput/cli/runCommand.h"
#include "autoinput/cli/commandBase.h"
#include <gtest/gtest.h>

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
        // Ensure parsing the command arguments with start and end mouse triggers succeeds
        ASSERT_TRUE(command.parse(gsl::make_span(argv.data(), argv.size()), index));
        // Ensure the parsed run command passes validation
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
        // Ensure post-parse argument processing succeeds for mouse triggers
        ASSERT_TRUE(arguments.postParseArguments());

        program.setBackend(std::make_unique<FakeBackend>());
        // Ensure program initializes successfully with fake backend
        ASSERT_TRUE(program.init());

        const auto& keyInfo = program.getKeyInfo();
        // Expecting 2 KeyInfo: one for start (back) and one for end (forward)
        // Ensure exactly two KeyInfo objects are created for start and end triggers
        ASSERT_EQ(keyInfo.size(), 2);

        // Start key info
        // Verify start trigger maps to Back mouse button
        EXPECT_EQ(keyInfo[0].triggerButton, MouseButton::Back);
        // Verify target action button is Left mouse button
        EXPECT_EQ(keyInfo[0].mouse.button, MouseButton::Left);
        // Verify entry is flagged as start key
        EXPECT_TRUE(keyInfo[0].isStartKey);

        // End key info
        // Verify end trigger maps to Forward mouse button
        EXPECT_EQ(keyInfo[1].triggerButton, MouseButton::Forward);
        // Verify entry is not flagged as start key
        EXPECT_FALSE(keyInfo[1].isStartKey);
    }
}
