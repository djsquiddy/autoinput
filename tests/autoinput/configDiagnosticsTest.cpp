/**
 * @file configDiagnosticsTest.cpp
 * @brief Unit tests for reusable non-UI configuration diagnostics and validation helpers.
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>

#include "autoinput/config/config.h"
#include "autoinput/config/configMetadata.h"
#include "autoinput_ui/graph/configDiagnostics.h"
#include "autoinput_ui/graph/configGraphAdapter.h"
#include "autoinput_ui/graph/graphModel.h"

using namespace autoinput;
using namespace autoinput::ui::graph;

class ConfigDiagnosticsTest : public ::testing::Test
{
protected:
    ConfigData createCleanConfig()
    {
        ConfigData config;
        config.application = "notepad.exe";
        config.blacklist = { "game.exe" };
        config.endKey = "escape";

        CommandData cmd1;
        cmd1.name = "FireWeapon";
        cmd1.action = "click";
        cmd1.startKeys = { "f1" };
        cmd1.keys = { "x" };
        cmd1.buttons = { "left" };
        cmd1.exclusiveGroup = "Combat";
        cmd1.pressWait = "20ms";
        cmd1.releaseWait = "40ms";
        cmd1.controls = { CommandControlData{ .action = "pause", .input = "p" } };

        CommandData cmd2;
        cmd2.name = "ReloadWeapon";
        cmd2.action = "click";
        cmd2.startKeys = { "f2" };
        cmd2.exclusiveGroup = "Combat";

        config.commands = { cmd1, cmd2 };

        RecordedSequence seq1;
        seq1.name = "ComboSeq";
        seq1.start = "f3";
        seq1.repeat = false;
        seq1.events = { RecordedEvent{ .type = RecordedEventType::KeyDown, .delay = "10ms", .key = "a" },
                        RecordedEvent{ .type = RecordedEventType::KeyUp, .delay = "10ms", .key = "a" } };

        config.sequences = { seq1 };
        return config;
    }
};

TEST_F(ConfigDiagnosticsTest, CleanConfigHasNoIssues)
{
    const auto config = createCleanConfig();
    const auto doc = configToGraphDocument(config);

    const auto result = analyzeConfigDiagnostics(config, &doc);

    EXPECT_FALSE(result.hasErrors());
    EXPECT_FALSE(result.hasWarnings());
    EXPECT_EQ(result.errorCount(), 0U);
    EXPECT_EQ(result.warningCount(), 0U);
}

TEST_F(ConfigDiagnosticsTest, SeverityHelperToString)
{
    EXPECT_EQ(configDiagnosticSeverityToString(ConfigDiagnosticSeverity::Info), "Info");
    EXPECT_EQ(configDiagnosticSeverityToString(ConfigDiagnosticSeverity::Warning), "Warning");
    EXPECT_EQ(configDiagnosticSeverityToString(ConfigDiagnosticSeverity::Error), "Error");
}

TEST_F(ConfigDiagnosticsTest, ResultCountersAndPredicates)
{
    ConfigDiagnosticsResult result;
    EXPECT_FALSE(result.hasErrors());
    EXPECT_FALSE(result.hasWarnings());
    EXPECT_FALSE(result.hasInfo());
    EXPECT_EQ(result.totalCount(), 0U);

    result.issues.push_back(ConfigDiagnosticIssue{ .severity = ConfigDiagnosticSeverity::Info, .message = "Info1" });
    EXPECT_FALSE(result.hasErrors());
    EXPECT_FALSE(result.hasWarnings());
    EXPECT_TRUE(result.hasInfo());
    EXPECT_EQ(result.infoCount(), 1U);
    EXPECT_EQ(result.totalCount(), 1U);

    result.issues.push_back(ConfigDiagnosticIssue{ .severity = ConfigDiagnosticSeverity::Warning, .message = "Warn1" });
    EXPECT_TRUE(result.hasWarnings());
    EXPECT_EQ(result.warningCount(), 1U);
    EXPECT_EQ(result.totalCount(), 2U);

    result.issues.push_back(ConfigDiagnosticIssue{ .severity = ConfigDiagnosticSeverity::Error, .message = "Err1" });
    EXPECT_TRUE(result.hasErrors());
    EXPECT_EQ(result.errorCount(), 1U);
    EXPECT_EQ(result.totalCount(), 3U);
}

TEST_F(ConfigDiagnosticsTest, DetectDuplicateCommandStartKeysWithinCommand)
{
    ConfigData config;
    CommandData cmd;
    cmd.name = "MultiKeyCmd";
    cmd.action = "click";
    cmd.startKeys = { "f1", "F1" };
    config.commands.push_back(cmd);

    const auto result = analyzeConfigDiagnostics(config);

    EXPECT_TRUE(result.hasWarnings());
    bool foundIssue = false;
    for (const auto& issue : result.issues)
    {
        if (issue.category == "Command" && issue.message.find("duplicate start key") != std::string::npos)
        {
            foundIssue = true;
            EXPECT_EQ(issue.severity, ConfigDiagnosticSeverity::Warning);
            EXPECT_EQ(issue.commandIndex, 0U);
            EXPECT_TRUE(issue.relatedInput.has_value());
            EXPECT_EQ(*issue.relatedInput, "F1");
        }
    }
    EXPECT_TRUE(foundIssue);
}

TEST_F(ConfigDiagnosticsTest, DetectDuplicateCommandStartKeysAcrossCommands)
{
    ConfigData config;
    CommandData cmd1;
    cmd1.name = "CmdA";
    cmd1.action = "click";
    cmd1.startKeys = { "f1" };

    CommandData cmd2;
    cmd2.name = "CmdB";
    cmd2.action = "click";
    cmd2.startKeys = { "F1" };

    config.commands = { cmd1, cmd2 };

    const auto result = analyzeConfigDiagnostics(config);

    EXPECT_TRUE(result.hasWarnings());
    bool foundConflict = false;
    for (const auto& issue : result.issues)
    {
        if (issue.category == "Input Conflict" &&
            issue.message.find("Duplicate command start input") != std::string::npos)
        {
            foundConflict = true;
            EXPECT_EQ(issue.severity, ConfigDiagnosticSeverity::Warning);
            EXPECT_TRUE(issue.relatedInput.has_value());
            EXPECT_EQ(*issue.relatedInput, "f1");
        }
    }
    EXPECT_TRUE(foundConflict);
}

TEST_F(ConfigDiagnosticsTest, DetectDuplicateSequenceStartKeys)
{
    ConfigData config;
    RecordedSequence seq1;
    seq1.name = "SeqA";
    seq1.start = "f8";

    RecordedSequence seq2;
    seq2.name = "SeqB";
    seq2.start = "f8";

    config.sequences = { seq1, seq2 };

    const auto result = analyzeConfigDiagnostics(config);

    EXPECT_TRUE(result.hasWarnings());
    bool foundConflict = false;
    for (const auto& issue : result.issues)
    {
        if (issue.category == "Input Conflict" &&
            issue.message.find("Duplicate sequence start input") != std::string::npos)
        {
            foundConflict = true;
            EXPECT_EQ(issue.severity, ConfigDiagnosticSeverity::Warning);
            EXPECT_TRUE(issue.relatedInput.has_value());
            EXPECT_EQ(*issue.relatedInput, "f8");
        }
    }
    EXPECT_TRUE(foundConflict);
}

TEST_F(ConfigDiagnosticsTest, DetectMixedCommandAndSequenceStartConflict)
{
    ConfigData config;
    CommandData cmd;
    cmd.name = "AttackCmd";
    cmd.action = "click";
    cmd.startKeys = { "f9" };
    config.commands.push_back(cmd);

    RecordedSequence seq;
    seq.name = "ComboSeq";
    seq.start = "f9";
    config.sequences.push_back(seq);

    const auto result = analyzeConfigDiagnostics(config);

    EXPECT_TRUE(result.hasWarnings());
    bool foundConflict = false;
    for (const auto& issue : result.issues)
    {
        if (issue.category == "Input Conflict" &&
            issue.message.find("Command and sequence start conflict") != std::string::npos)
        {
            foundConflict = true;
            EXPECT_EQ(issue.severity, ConfigDiagnosticSeverity::Warning);
            EXPECT_EQ(issue.commandIndex, 0U);
            EXPECT_EQ(issue.sequenceIndex, 0U);
            EXPECT_TRUE(issue.relatedInput.has_value());
            EXPECT_EQ(*issue.relatedInput, "f9");
            EXPECT_NE(issue.message.find("AttackCmd"), std::string::npos);
            EXPECT_NE(issue.message.find("ComboSeq"), std::string::npos);
        }
    }
    EXPECT_TRUE(foundConflict);
}

TEST_F(ConfigDiagnosticsTest, DetectEmptyCommandName)
{
    ConfigData config;
    CommandData cmd;
    cmd.name = "";
    cmd.action = "click";
    config.commands.push_back(cmd);

    const auto result = analyzeConfigDiagnostics(config);

    EXPECT_TRUE(result.hasWarnings());
    bool foundEmptyName = false;
    for (const auto& issue : result.issues)
    {
        if (issue.category == "Command" && issue.message.find("empty name") != std::string::npos)
        {
            foundEmptyName = true;
            EXPECT_EQ(issue.commandIndex, 0U);
            EXPECT_EQ(issue.severity, ConfigDiagnosticSeverity::Warning);
        }
    }
    EXPECT_TRUE(foundEmptyName);
}

TEST_F(ConfigDiagnosticsTest, DetectDuplicateCommandNames)
{
    ConfigData config;
    CommandData cmd1;
    cmd1.name = "SpecialMove";
    cmd1.action = "click";

    CommandData cmd2;
    cmd2.name = "specialmove"; // Duplicate name case-insensitively
    cmd2.action = "click";

    config.commands = { cmd1, cmd2 };

    const auto result = analyzeConfigDiagnostics(config);

    EXPECT_TRUE(result.hasWarnings());
    bool foundDuplicateName = false;
    for (const auto& issue : result.issues)
    {
        if (issue.category == "Command" && issue.message.find("Duplicate command name") != std::string::npos)
        {
            foundDuplicateName = true;
            EXPECT_EQ(issue.severity, ConfigDiagnosticSeverity::Warning);
        }
    }
    EXPECT_TRUE(foundDuplicateName);
}

TEST_F(ConfigDiagnosticsTest, DetectEmptyCommandAction)
{
    ConfigData config;
    CommandData cmd;
    cmd.name = "ActionlessCmd";
    cmd.action = "";
    config.commands.push_back(cmd);

    const auto result = analyzeConfigDiagnostics(config);

    EXPECT_TRUE(result.hasErrors());
    bool foundEmptyAction = false;
    for (const auto& issue : result.issues)
    {
        if (issue.category == "Command" && issue.message.find("no action specified") != std::string::npos)
        {
            foundEmptyAction = true;
            EXPECT_EQ(issue.severity, ConfigDiagnosticSeverity::Error);
            EXPECT_EQ(issue.commandIndex, 0U);
        }
    }
    EXPECT_TRUE(foundEmptyAction);
}

TEST_F(ConfigDiagnosticsTest, DetectInvalidCommandActionUsingMetadata)
{
    ConfigData config;
    CommandData cmd;
    cmd.name = "InvalidActionCmd";
    cmd.action = "teleport"; // Not a valid action
    config.commands.push_back(cmd);

    const auto result = analyzeConfigDiagnostics(config);

    EXPECT_TRUE(result.hasErrors());
    bool foundInvalidAction = false;
    for (const auto& issue : result.issues)
    {
        if (issue.category == "Command" && issue.message.find("invalid action 'teleport'") != std::string::npos)
        {
            foundInvalidAction = true;
            EXPECT_EQ(issue.severity, ConfigDiagnosticSeverity::Error);
            EXPECT_EQ(issue.commandIndex, 0U);
        }
    }
    EXPECT_TRUE(foundInvalidAction);
}

TEST_F(ConfigDiagnosticsTest, DetectEmptyControlsAndInvalidControlActions)
{
    ConfigData config;
    CommandData cmd;
    cmd.name = "ControlHostCmd";
    cmd.action = "click";
    cmd.controls = {
        CommandControlData{ .action = "", .input = "" },               // Completely empty
        CommandControlData{ .action = "pause", .input = "" },          // Empty input
        CommandControlData{ .action = "", .input = "k" },              // Empty action
        CommandControlData{ .action = "invalid_action", .input = "j" } // Invalid action
    };
    config.commands.push_back(cmd);

    const auto result = analyzeConfigDiagnostics(config);

    EXPECT_TRUE(result.hasErrors());
    EXPECT_TRUE(result.hasWarnings());

    bool foundCompletelyEmpty = false;
    bool foundEmptyInput = false;
    bool foundEmptyAction = false;
    bool foundInvalidAction = false;

    for (const auto& issue : result.issues)
    {
        if (issue.category == "Control")
        {
            if (issue.controlIndex == 0U && issue.message.find("is empty") != std::string::npos)
            {
                foundCompletelyEmpty = true;
                EXPECT_EQ(issue.severity, ConfigDiagnosticSeverity::Error);
            }
            if (issue.controlIndex == 1U && issue.message.find("empty input binding") != std::string::npos)
            {
                foundEmptyInput = true;
                EXPECT_EQ(issue.severity, ConfigDiagnosticSeverity::Error);
            }
            if (issue.controlIndex == 2U && issue.message.find("no action specified") != std::string::npos)
            {
                foundEmptyAction = true;
                EXPECT_EQ(issue.severity, ConfigDiagnosticSeverity::Warning);
                EXPECT_EQ(issue.relatedInput, "k");
            }
            if (issue.controlIndex == 3U && issue.message.find("invalid action 'invalid_action'") != std::string::npos)
            {
                foundInvalidAction = true;
                EXPECT_EQ(issue.severity, ConfigDiagnosticSeverity::Error);
                EXPECT_EQ(issue.relatedInput, "j");
            }
        }
    }

    EXPECT_TRUE(foundCompletelyEmpty);
    EXPECT_TRUE(foundEmptyInput);
    EXPECT_TRUE(foundEmptyAction);
    EXPECT_TRUE(foundInvalidAction);
}

TEST_F(ConfigDiagnosticsTest, DetectEmptySequenceNameAndEmptyStartKey)
{
    ConfigData config;
    RecordedSequence seq;
    seq.name = "";
    seq.start = "";
    config.sequences.push_back(seq);

    const auto result = analyzeConfigDiagnostics(config);

    EXPECT_TRUE(result.hasWarnings());
    bool foundEmptyName = false;
    bool foundEmptyStart = false;

    for (const auto& issue : result.issues)
    {
        if (issue.category == "Sequence")
        {
            if (issue.message.find("empty name") != std::string::npos)
            {
                foundEmptyName = true;
                EXPECT_EQ(issue.sequenceIndex, 0U);
                EXPECT_EQ(issue.severity, ConfigDiagnosticSeverity::Warning);
            }
            if (issue.message.find("no start trigger key configured") != std::string::npos)
            {
                foundEmptyStart = true;
                EXPECT_EQ(issue.sequenceIndex, 0U);
                EXPECT_EQ(issue.severity, ConfigDiagnosticSeverity::Warning);
            }
        }
    }

    EXPECT_TRUE(foundEmptyName);
    EXPECT_TRUE(foundEmptyStart);
}

TEST_F(ConfigDiagnosticsTest, DetectDuplicateSequenceNames)
{
    ConfigData config;
    RecordedSequence seq1;
    seq1.name = "AutoFarm";
    seq1.start = "f4";

    RecordedSequence seq2;
    seq2.name = "autofarm"; // Duplicate
    seq2.start = "f5";

    config.sequences = { seq1, seq2 };

    const auto result = analyzeConfigDiagnostics(config);

    EXPECT_TRUE(result.hasWarnings());
    bool foundDuplicateSeqName = false;
    for (const auto& issue : result.issues)
    {
        if (issue.category == "Sequence" && issue.message.find("Duplicate sequence name") != std::string::npos)
        {
            foundDuplicateSeqName = true;
            EXPECT_EQ(issue.severity, ConfigDiagnosticSeverity::Warning);
        }
    }
    EXPECT_TRUE(foundDuplicateSeqName);
}

TEST_F(ConfigDiagnosticsTest, DetectWildcardControlInformationalDiagnostics)
{
    ConfigData config;
    CommandData cmd;
    cmd.name = "GlobalPauseCmd";
    cmd.action = "click";
    cmd.controls = { CommandControlData{ .action = "pause", .input = "input.all" },
                     CommandControlData{ .action = "stop", .input = "mouse.all" },
                     CommandControlData{ .action = "cancel", .input = "keys.all" } };
    config.commands.push_back(cmd);

    const auto result = analyzeConfigDiagnostics(config);

    EXPECT_TRUE(result.hasInfo());
    std::size_t wildcardCount = 0;
    for (const auto& issue : result.issues)
    {
        if (issue.category == "Wildcard Control")
        {
            wildcardCount++;
            EXPECT_EQ(issue.severity, ConfigDiagnosticSeverity::Info);
            EXPECT_EQ(issue.commandIndex, 0U);
            EXPECT_TRUE(issue.relatedInput.has_value());
            EXPECT_TRUE(issue.message.find("wildcard input trigger") != std::string::npos);
        }
    }
    EXPECT_EQ(wildcardCount, 3U);
}

TEST_F(ConfigDiagnosticsTest, DetectCommandsSharingInputKeysOrButtons)
{
    ConfigData config;
    CommandData cmd1;
    cmd1.name = "FirstCmd";
    cmd1.action = "click";
    cmd1.keys = { "space" };

    CommandData cmd2;
    cmd2.name = "SecondCmd";
    cmd2.action = "click";
    cmd2.keys = { "space" };

    config.commands = { cmd1, cmd2 };

    const auto result = analyzeConfigDiagnostics(config);

    EXPECT_TRUE(result.hasInfo());
    bool foundSharedInput = false;
    for (const auto& issue : result.issues)
    {
        if (issue.category == "Input Conflict" && issue.message.find("share input 'space'") != std::string::npos)
        {
            foundSharedInput = true;
            EXPECT_EQ(issue.severity, ConfigDiagnosticSeverity::Info);
            EXPECT_TRUE(issue.relatedInput.has_value());
            EXPECT_EQ(*issue.relatedInput, "space");
        }
    }
    EXPECT_TRUE(foundSharedInput);
}

TEST_F(ConfigDiagnosticsTest, MapDiagnosticsToGraphDocumentNodes)
{
    ConfigData config;
    CommandData cmd;
    cmd.name = ""; // Triggers empty name warning
    cmd.action = "click";
    config.commands.push_back(cmd);

    RecordedSequence seq;
    seq.name = ""; // Triggers empty name warning
    seq.start = "";
    config.sequences.push_back(seq);

    const auto doc = configToGraphDocument(config);
    const auto result = analyzeConfigDiagnostics(config, &doc);

    bool mappedCommandNode = false;
    bool mappedSequenceNode = false;

    for (const auto& issue : result.issues)
    {
        if (issue.commandIndex == 0U && issue.associatedNodeId.has_value())
        {
            const auto* node = doc.findNode(*issue.associatedNodeId);
            ASSERT_NE(node, nullptr);
            EXPECT_EQ(node->kind, NodeKind::Command);
            mappedCommandNode = true;
        }
        if (issue.sequenceIndex == 0U && issue.associatedNodeId.has_value())
        {
            const auto* node = doc.findNode(*issue.associatedNodeId);
            ASSERT_NE(node, nullptr);
            EXPECT_EQ(node->kind, NodeKind::Sequence);
            mappedSequenceNode = true;
        }
    }

    EXPECT_TRUE(mappedCommandNode);
    EXPECT_TRUE(mappedSequenceNode);
}
