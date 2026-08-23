/**
 * @file serveCommandTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <gsl/gsl>

#include "autoinput/cli/serveCommand.h"

namespace autoinput::cli
{
    namespace
    {
        std::vector<char*> toArgv(const std::vector<std::string>& args)
        {
            std::vector<char*> argv;
            argv.reserve(args.size());
            for (const auto& arg : args)
            {
                argv.push_back(const_cast<char*>(arg.c_str()));
            }
            return argv;
        }
    }

    TEST(ServeCommandTest, BasicProperties)
    {
        CommandContext context;
        ServeCommand cmd(context);
        // Verify the command name is 'serve'
        EXPECT_EQ(cmd.getName(), "serve");
        
        HelpEntry help = cmd.getHelpEntry();
        // Verify the command help usage string is not empty
        EXPECT_FALSE(help.usage.empty());
        // Verify the command help description is not empty
        EXPECT_FALSE(help.description.empty());
    }

    TEST(ServeCommandTest, PositiveParseValidate)
    {
        CommandContext context;
        const std::vector<std::vector<std::string>> cases = {
            {"serve", "--stdio"}
        };

        for (const auto& args : cases)
        {
            ServeCommand cmd(context);
            auto argv = toArgv(args);
            i32 index = 1;
            // Verify parsing valid serve command arguments succeeds
            EXPECT_TRUE(cmd.parse(gsl::make_span(argv), index)) << "Failed to parse: " << args[1];
            // Verify validating parsed serve command succeeds
            EXPECT_TRUE(cmd.validate()) << "Failed to validate: " << args[1];
        }
    }

    TEST(ServeCommandTest, NegativeParseValidate)
    {
        CommandContext context;
        const std::vector<std::vector<std::string>> cases = {
            {"serve"},             // Missing --stdio
            {"serve", "--unknown"} // Unknown argument
        };

        for (const auto& args : cases)
        {
            ServeCommand cmd(context);
            auto argv = toArgv(args);
            i32 index = 1;
            if (cmd.parse(gsl::make_span(argv), index))
            {
                // Verify validation fails when mandatory flags like --stdio are missing
                EXPECT_FALSE(cmd.validate()) << "Should have failed validation";
            }
        }
    }
}
