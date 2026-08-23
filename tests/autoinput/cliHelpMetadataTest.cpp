/**
 * @file cliHelpMetadataTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include <algorithm>

#include "autoinput/cli/cliHelpMetadata.h"
#include "autoinput/cli/commandBase.h"
#include "autoinput/cli/configCommand.h"

namespace autoinput::cli
{
    TEST(CliHelpMetadataTest, FindCommandFindsAllTopLevelCommands)
    {
        for (const std::string_view name : { "run", "record", "config", "apps", "serve", "help" })
        {
            const auto* command = HelpMetadata::findCommand(name);
            // Verify top-level command metadata entry exists and command name matches
            ASSERT_NE(command, nullptr) << "Missing top-level command: " << name;
            EXPECT_EQ(command->name, name);
        }
    }

    TEST(CliHelpMetadataTest, FindCommandReturnsNullForUnknownCommand)
    {
        // Verify looking up non-existent command returns nullptr
        EXPECT_EQ(HelpMetadata::findCommand("not-a-real-command"), nullptr);
    }

    TEST(CliHelpMetadataTest, ConfigCommandHasExpectedSubcommandsWithCorrectUsage)
    {
        const auto* config = HelpMetadata::findCommand("config");
        // Verify config command metadata exists
        ASSERT_NE(config, nullptr);

        struct Expected
        {
            std::string_view name;
            std::string_view usage;
        };

        const std::array<Expected, 5> expected{ {
            { "list", "list" },
            { "validate", "validate NAME_OR_PATH" },
            { "duplicate", "duplicate SOURCE DESTINATION" },
            { "copy", "copy SOURCE DESTINATION" },
            { "path", "path NAME_OR_PATH" },
        } };

        // Verify config command has expected number of subcommands
        EXPECT_EQ(config->subcommands.size(), expected.size());

        for (const auto& [name, usage] : expected)
        {
            const auto* sub = HelpMetadata::findSubcommand(*config, name);
            // Verify each subcommand exists and usage string matches metadata
            ASSERT_NE(sub, nullptr) << "Missing config subcommand: " << name;
            EXPECT_EQ(sub->usage, usage) << "Unexpected usage for config subcommand: " << name;
        }
    }

    TEST(CliHelpMetadataTest, FindSubcommandReturnsNullForUnknownSubcommand)
    {
        const auto* config = HelpMetadata::findCommand("config");
        // Verify config metadata exists and invalid subcommand lookup returns nullptr
        ASSERT_NE(config, nullptr);
        EXPECT_EQ(HelpMetadata::findSubcommand(*config, "not-a-real-subcommand"), nullptr);
    }

    TEST(CliHelpMetadataTest, GlobalOptionsMatchExpectedFlags)
    {
        // Verify global options count
        ASSERT_EQ(HelpMetadata::GLOBAL_OPTIONS.size(), 4u);

        const auto hasFlag = [](const HelpMetadata::CliOptionMetadata& option, const std::string_view flag)
        {
            return std::ranges::find(option.names, flag) != option.names.end();
        };

        // Verify expected flag names for each global option entry
        EXPECT_TRUE(hasFlag(HelpMetadata::GLOBAL_OPTIONS[0], "-h"));
        EXPECT_TRUE(hasFlag(HelpMetadata::GLOBAL_OPTIONS[0], "--help"));
        EXPECT_TRUE(hasFlag(HelpMetadata::GLOBAL_OPTIONS[1], "--examples"));
        EXPECT_TRUE(hasFlag(HelpMetadata::GLOBAL_OPTIONS[2], "-l"));
        EXPECT_TRUE(hasFlag(HelpMetadata::GLOBAL_OPTIONS[2], "--log"));
        EXPECT_TRUE(hasFlag(HelpMetadata::GLOBAL_OPTIONS[3], "--json"));
    }

    TEST(CliHelpMetadataTest, ConfigCommandGetHelpEntryMatchesGeneratedMetadata)
    {
        CommandContext context;
        ConfigCommand cmd(context);

        const HelpEntry entry = cmd.getHelpEntry();
        const auto* metadata = HelpMetadata::findCommand("config");
        // Verify config command metadata exists
        ASSERT_NE(metadata, nullptr);

        // Verify help entry usage and description match generated metadata
        EXPECT_EQ(entry.usage, metadata->usage);
        EXPECT_EQ(entry.description, metadata->description);
    }

    TEST(CliHelpMetadataTest, ConfigCommandGetHelpEntryForSubcommandMatchesGeneratedMetadata)
    {
        CommandContext context;
        ConfigCommand cmd(context);
        cmd.setHelpTopic({ "config", "validate" });

        const HelpEntry entry = cmd.getHelpEntry();
        const auto* configMetadata = HelpMetadata::findCommand("config");
        // Verify config command metadata and validate subcommand metadata exist
        ASSERT_NE(configMetadata, nullptr);
        const auto* validateMetadata = HelpMetadata::findSubcommand(*configMetadata, "validate");
        ASSERT_NE(validateMetadata, nullptr);

        // Verify help entry formatted usage and description match validate subcommand metadata
        EXPECT_EQ(entry.usage, std::format("config {}", validateMetadata->usage));
        EXPECT_EQ(entry.description, validateMetadata->description);
    }
}
