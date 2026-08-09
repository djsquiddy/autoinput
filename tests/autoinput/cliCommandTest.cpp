/**
 * @file cliCommandTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <gsl/gsl>

#include "autoinput/errorCode.h"
#include "autoinput/cli/cliApplication.h"
#include "autoinput/cli/runCommand.h"
#include "autoinput/cli/recordCommand.h"
#include "autoinput/cli/configCommand.h"
#include "autoinput/cli/appsCommand.h"
#include "autoinput/cli/helpCommand.h"

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

    TEST(CliApplicationTest, ParsesHelpCommands)
    {
        const std::vector<std::vector<std::string>> cases = {
            {"autoinput"},
            {"autoinput", "help"},
            {"autoinput", "help", "run"},
            {"autoinput", "help", "record"},
            {"autoinput", "help", "config"},
            {"autoinput", "help", "config", "validate"},
            {"autoinput", "help", "apps"},
            {"autoinput", "help", "serve"},
            {"autoinput", "--json", "config", "validate", "my-config"}
        };

        for (const auto& args : cases)
        {
            CliApplication app;
            auto argv = toArgv(args);
            EXPECT_TRUE(app.parse(gsl::make_span(argv)) == ErrorCode::Success) << "Failed to parse: " << args[1];
        }
    }

    TEST(CliApplicationTest, FailsOnUnknownCommandOrOption)
    {
        CliApplication app;
        std::vector<std::string> args = {"autoinput", "--bad"};
        auto argv = toArgv(args);
        EXPECT_FALSE(app.parse(gsl::make_span(argv)) == ErrorCode::Success);

        args = {"autoinput", "unknown-command"};
        argv = toArgv(args);
        EXPECT_FALSE(app.parse(gsl::make_span(argv)) == ErrorCode::Success);
    }

    TEST(RunCommandTest, PositiveParseValidate)
    {
        CommandContext context;
        const std::vector<std::vector<std::string>> cases = {
            {"run"},
            {"run", "--button", "left"},
            {"run", "--type", "hold", "--button", "left", "--start", "f2"},
            {"run", "--key", "space", "--start", "f6"},
            {"run", "--config", "my-config"},
            {"run", "--button", "left", "--save-config", "my-setup"},
            {"run", "--button", "left", "--press-wait", "100ms..200ms"},
            {"run", "--button", "left", "--release-wait", "1s"},
            {"run", "--app", "game.exe"},
            {"run", "--blacklist", "overlay.exe"}
        };

        for (const auto& args : cases)
        {
            RunCommand cmd(context);
            auto argv = toArgv(args);
            i32 index = 1;
            EXPECT_TRUE(cmd.parse(gsl::make_span(argv), index)) << "Failed to parse: " << args[0];
            EXPECT_TRUE(cmd.validate()) << "Failed to validate: " << args[0];
        }
    }

    TEST(RunCommandTest, NegativeParseValidate)
    {
        CommandContext context;
        const std::vector<std::vector<std::string>> cases = {
            {"run", "--type"},
            {"run", "--type", "invalid"},
            {"run", "--button"},
            {"run", "--button", "invalid"},
            {"run", "--key"},
            {"run", "--start"},
            {"run", "--end"},
            {"run", "--app"},
            {"run", "--blacklist"},
            {"run", "--press-wait", "invalid"},
            {"run", "--release-wait", "invalid"},
            {"run", "--unknown"}
        };

        for (const auto& args : cases)
        {
            RunCommand cmd(context);
            auto argv = toArgv(args);
            i32 index = 1;
            // parse might return true but validate false, or parse might return false
            if (cmd.parse(gsl::make_span(argv), index))
            {
                EXPECT_FALSE(cmd.validate()) << "Should have failed validation: " << args[1];
            }
        }
    }

    TEST(RecordCommandTest, PositiveParseValidate)
    {
        CommandContext context;
        const std::vector<std::vector<std::string>> cases = {
            {"record", "my-macro"},
            {"record", "my-macro", "--start", "f8"},
            {"record", "my-macro", "--end", "f9"},
            {"record", "my-macro", "--play-start", "f6"},
            {"record", "my-macro", "--mouse-moves"},
            {"record", "my-macro", "--mouse-sample", "25ms"},
            {"record", "my-macro", "--force"},
            {"record", "my-macro", "--start", "f8", "--end", "f9", "--play-start", "f6", "--mouse-moves", "--mouse-sample", "25ms", "--force"}
        };

        for (const auto& args : cases)
        {
            RecordCommand cmd(context);
            auto argv = toArgv(args);
            i32 index = 1;
            EXPECT_TRUE(cmd.parse(gsl::make_span(argv), index)) << "Failed to parse: " << args[1];
            EXPECT_TRUE(cmd.validate()) << "Failed to validate: " << args[1];
        }
    }

    TEST(RecordCommandTest, NegativeParseValidate)
    {
        CommandContext context;
        const std::vector<std::vector<std::string>> cases = {
            {"record"},
            {"record", "--start", "f8"},
            {"record", "my-macro", "--start"},
            {"record", "my-macro", "--end"},
            {"record", "my-macro", "--play-start"},
            {"record", "my-macro", "--mouse-sample"},
            {"record", "my-macro", "--unknown"}
        };

        for (const auto& args : cases)
        {
            RecordCommand cmd(context);
            auto argv = toArgv(args);
            i32 index = 1;
            if (cmd.parse(gsl::make_span(argv), index))
            {
                EXPECT_FALSE(cmd.validate()) << "Should have failed validation";
            }
        }
    }

    TEST(ConfigCommandTest, PositiveParseValidate)
    {
        CommandContext context;
        const std::vector<std::vector<std::string>> cases = {
            {"config", "list"},
            {"config", "validate", "my-config"},
            {"config", "duplicate", "source", "dest"},
            {"config", "duplicate", "source", "dest", "--force"},
            {"config", "copy", "source", "dest"},
            {"config", "copy", "source", "dest", "--force"}
        };

        for (const auto& args : cases)
        {
            ConfigCommand cmd(context);
            auto argv = toArgv(args);
            i32 index = 1;
            EXPECT_TRUE(cmd.parse(gsl::make_span(argv), index)) << "Failed to parse: " << args[1];
            EXPECT_TRUE(cmd.validate()) << "Failed to validate: " << args[1];
        }
    }

    TEST(ConfigCommandTest, NegativeParseValidate)
    {
        CommandContext context;
        const std::vector<std::vector<std::string>> cases = {
            {"config"},
            {"config", "unknown"},
            {"config", "validate"},
            {"config", "duplicate"},
            {"config", "duplicate", "source"},
            {"config", "copy"},
            {"config", "copy", "source"},
            {"config", "list", "--force"},
            {"config", "validate", "my-config", "--force"},
            {"config", "duplicate", "source", "dest", "--bad"}
        };

        for (const auto& args : cases)
        {
            ConfigCommand cmd(context);
            auto argv = toArgv(args);
            i32 index = 1;
            if (cmd.parse(gsl::make_span(argv), index))
            {
                EXPECT_FALSE(cmd.validate()) << "Should have failed validation";
            }
        }
    }

    TEST(AppsCommandTest, PositiveParseValidate)
    {
        CommandContext context;
        const std::vector<std::vector<std::string>> cases = {
            {"apps", "list"}
        };

        for (const auto& args : cases)
        {
            AppsCommand cmd(context);
            auto argv = toArgv(args);
            i32 index = 1;
            EXPECT_TRUE(cmd.parse(gsl::make_span(argv), index)) << "Failed to parse: " << args[1];
            EXPECT_TRUE(cmd.validate()) << "Failed to validate: " << args[1];
        }
    }

    TEST(AppsCommandTest, NegativeParseValidate)
    {
        CommandContext context;
        const std::vector<std::vector<std::string>> cases = {
            {"apps"},
            {"apps", "unknown"},
            {"apps", "list", "extra"}
        };

        for (const auto& args : cases)
        {
            AppsCommand cmd(context);
            auto argv = toArgv(args);
            i32 index = 1;
            if (cmd.parse(gsl::make_span(argv), index))
            {
                EXPECT_FALSE(cmd.validate()) << "Should have failed validation";
            }
        }
    }

    TEST(HelpCommandTest, PositiveParseValidate)
    {
        CommandContext context;
        const std::vector<std::vector<std::string>> cases = {
            {"help"},
            {"help", "run"},
            {"help", "record"},
            {"help", "config"},
            {"help", "config", "list"},
            {"help", "config", "validate"},
            {"help", "config", "duplicate"},
            {"help", "config", "copy"},
            {"help", "apps"}
        };

        for (const auto& args : cases)
        {
            HelpCommand cmd(context);
            auto argv = toArgv(args);
            i32 index = 1;
            EXPECT_TRUE(cmd.parse(gsl::make_span(argv), index)) << "Failed to parse: " << args[1];
            EXPECT_TRUE(cmd.validate()) << "Failed to validate: " << args[1];
        }
    }

    TEST(HelpCommandTest, NegativeParseValidate)
    {
        CommandContext context;
        const std::vector<std::vector<std::string>> cases = {
            {"help", "unknown"},
            {"help", "run", "extra"},
            {"help", "config", "unknown"}
        };

        for (const auto& args : cases)
        {
            HelpCommand cmd(context);
            auto argv = toArgv(args);
            i32 index = 1;
            if (cmd.parse(gsl::make_span(argv), index))
            {
                EXPECT_FALSE(cmd.validate()) << "Should have failed validation";
            }
        }
    }
}
