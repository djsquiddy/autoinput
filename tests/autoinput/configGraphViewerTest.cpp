/**
 * @file configGraphViewerTest.cpp
 * @brief Unit tests for the read-only config graph viewer and diagnostics logic.
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>

#include "autoinput/config/config.h"
#include "autoinput_ui/editors/configGraphViewer.h"
#include "autoinput_ui/graph/configGraphAdapter.h"
#include "autoinput_ui/graph/graphModel.h"

using namespace autoinput;
using namespace autoinput::ui::editors;
using namespace autoinput::ui::graph;

class ConfigGraphViewerTest : public ::testing::Test
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

TEST_F(ConfigGraphViewerTest, CleanConfigDiagnostics)
{
    auto config = createCleanConfig();
    auto doc = configToGraphDocument(config);

    auto result = analyzeConfigDiagnostics(config, &doc);

    // Clean config should have 0 errors and 0 warnings
    EXPECT_FALSE(result.hasErrors());
    EXPECT_FALSE(result.hasWarnings());
    EXPECT_EQ(result.errorCount(), 0U);
    EXPECT_EQ(result.warningCount(), 0U);
}

TEST_F(ConfigGraphViewerTest, DetectDuplicateStartKeysAcrossCommands)
{
    ConfigData config;

    CommandData cmd1;
    cmd1.name = "Cmd1";
    cmd1.action = "press";
    cmd1.startKeys = { "f1" };

    CommandData cmd2;
    cmd2.name = "Cmd2";
    cmd2.action = "press";
    cmd2.startKeys = { "F1" }; // Same key, different casing

    config.commands = { cmd1, cmd2 };

    auto doc = configToGraphDocument(config);
    auto result = analyzeConfigDiagnostics(config, &doc);

    EXPECT_TRUE(result.hasWarnings());
    bool foundConflict = false;
    for (const auto& issue : result.issues)
    {
        if (issue.category == "Input Conflict" && issue.message.find("Duplicate start input") != std::string::npos)
        {
            foundConflict = true;
            EXPECT_EQ(issue.severity, ConfigDiagnosticSeverity::Warning);
            EXPECT_NE(issue.message.find("f1"), std::string::npos);
            break;
        }
    }
    EXPECT_TRUE(foundConflict);
}

TEST_F(ConfigGraphViewerTest, DetectDuplicateStartKeysBetweenCommandAndSequence)
{
    ConfigData config;

    CommandData cmd;
    cmd.name = "PrimaryCmd";
    cmd.action = "press";
    cmd.startKeys = { "f5" };
    config.commands.push_back(cmd);

    RecordedSequence seq;
    seq.name = "PrimarySeq";
    seq.start = "f5";
    config.sequences.push_back(seq);

    auto doc = configToGraphDocument(config);
    auto result = analyzeConfigDiagnostics(config, &doc);

    EXPECT_TRUE(result.hasWarnings());
    bool foundConflict = false;
    for (const auto& issue : result.issues)
    {
        if (issue.category == "Input Conflict" && issue.message.find("Duplicate start input") != std::string::npos)
        {
            foundConflict = true;
            EXPECT_NE(issue.message.find("PrimaryCmd"), std::string::npos);
            EXPECT_NE(issue.message.find("PrimarySeq"), std::string::npos);
            break;
        }
    }
    EXPECT_TRUE(foundConflict);
}

TEST_F(ConfigGraphViewerTest, DetectDuplicateStartKeysWithinSameCommand)
{
    ConfigData config;

    CommandData cmd;
    cmd.name = "MultiKeyCmd";
    cmd.action = "press";
    cmd.startKeys = { "f1", "F1" }; // Duplicate within same command
    config.commands.push_back(cmd);

    auto doc = configToGraphDocument(config);
    auto result = analyzeConfigDiagnostics(config, &doc);

    EXPECT_TRUE(result.hasWarnings());
    bool foundDuplicateInCmd = false;
    for (const auto& issue : result.issues)
    {
        if (issue.category == "Command" && issue.message.find("duplicate start key") != std::string::npos)
        {
            foundDuplicateInCmd = true;
            EXPECT_EQ(issue.commandIndex, 0U);
            break;
        }
    }
    EXPECT_TRUE(foundDuplicateInCmd);
}

TEST_F(ConfigGraphViewerTest, DetectCommandsSharingInputKeysOrButtons)
{
    ConfigData config;

    CommandData cmd1;
    cmd1.name = "FirstCmd";
    cmd1.action = "press";
    cmd1.keys = { "space" };

    CommandData cmd2;
    cmd2.name = "SecondCmd";
    cmd2.action = "press";
    cmd2.keys = { "space" };

    config.commands = { cmd1, cmd2 };

    auto doc = configToGraphDocument(config);
    auto result = analyzeConfigDiagnostics(config, &doc);

    bool foundSharedInput = false;
    for (const auto& issue : result.issues)
    {
        if (issue.category == "Input Conflict" && issue.message.find("share input 'space'") != std::string::npos)
        {
            foundSharedInput = true;
            EXPECT_EQ(issue.severity, ConfigDiagnosticSeverity::Info);
            break;
        }
    }
    EXPECT_TRUE(foundSharedInput);
}

TEST_F(ConfigGraphViewerTest, DetectEmptyCommandNameAndMissingAction)
{
    ConfigData config;

    CommandData cmd;
    cmd.name = "";   // Empty name
    cmd.action = ""; // Missing action
    config.commands.push_back(cmd);

    auto doc = configToGraphDocument(config);
    auto result = analyzeConfigDiagnostics(config, &doc);

    EXPECT_TRUE(result.hasErrors());
    EXPECT_TRUE(result.hasWarnings());

    bool foundEmptyName = false;
    bool foundMissingAction = false;
    for (const auto& issue : result.issues)
    {
        if (issue.message.find("empty name") != std::string::npos)
        {
            foundEmptyName = true;
            EXPECT_EQ(issue.severity, ConfigDiagnosticSeverity::Warning);
            EXPECT_EQ(issue.commandIndex, 0U);
        }
        if (issue.message.find("no action specified") != std::string::npos)
        {
            foundMissingAction = true;
            EXPECT_EQ(issue.severity, ConfigDiagnosticSeverity::Error);
            EXPECT_EQ(issue.commandIndex, 0U);
        }
    }
    EXPECT_TRUE(foundEmptyName);
    EXPECT_TRUE(foundMissingAction);
}

TEST_F(ConfigGraphViewerTest, DetectControlWithEmptyInputOrAction)
{
    ConfigData config;

    CommandData cmd;
    cmd.name = "CtrlTest";
    cmd.action = "press";
    cmd.controls = {
        CommandControlData{ .action = "pause", .input = "" }, // Empty input
        CommandControlData{ .action = "", .input = "key1" }   // Empty action
    };
    config.commands.push_back(cmd);

    auto doc = configToGraphDocument(config);
    auto result = analyzeConfigDiagnostics(config, &doc);

    EXPECT_TRUE(result.hasErrors());
    bool foundEmptyInput = false;
    bool foundEmptyAction = false;

    for (const auto& issue : result.issues)
    {
        if (issue.category == "Control" && issue.message.find("empty input binding") != std::string::npos)
        {
            foundEmptyInput = true;
            EXPECT_EQ(issue.severity, ConfigDiagnosticSeverity::Error);
            EXPECT_EQ(issue.commandIndex, 0U);
            EXPECT_EQ(issue.controlIndex, 0U);
        }
        if (issue.category == "Control" && issue.message.find("no action specified") != std::string::npos)
        {
            foundEmptyAction = true;
            EXPECT_EQ(issue.severity, ConfigDiagnosticSeverity::Warning);
            EXPECT_EQ(issue.commandIndex, 0U);
            EXPECT_EQ(issue.controlIndex, 1U);
        }
    }
    EXPECT_TRUE(foundEmptyInput);
    EXPECT_TRUE(foundEmptyAction);
}

TEST_F(ConfigGraphViewerTest, DetectSequenceWithEmptyNameOrStart)
{
    ConfigData config;

    RecordedSequence seq;
    seq.name = "";  // Empty name
    seq.start = ""; // Empty start
    config.sequences.push_back(seq);

    auto doc = configToGraphDocument(config);
    auto result = analyzeConfigDiagnostics(config, &doc);

    EXPECT_TRUE(result.hasWarnings());
    bool foundEmptyName = false;
    bool foundEmptyStart = false;

    for (const auto& issue : result.issues)
    {
        if (issue.category == "Sequence" && issue.message.find("empty name") != std::string::npos)
        {
            foundEmptyName = true;
            EXPECT_EQ(issue.sequenceIndex, 0U);
        }
        if (issue.category == "Sequence" && issue.message.find("no start trigger key") != std::string::npos)
        {
            foundEmptyStart = true;
            EXPECT_EQ(issue.sequenceIndex, 0U);
        }
    }
    EXPECT_TRUE(foundEmptyName);
    EXPECT_TRUE(foundEmptyStart);
}

TEST_F(ConfigGraphViewerTest, InspectConfigGraphNodeDetails)
{
    auto config = createCleanConfig();
    auto doc = configToGraphDocument(config);
    auto diag = analyzeConfigDiagnostics(config, &doc);

    // Find command node
    NodeId cmdNodeId = InvalidNodeId;
    for (const auto& n : doc.nodes())
    {
        if (n.kind == NodeKind::Command && n.sourceIndex == 0U)
        {
            cmdNodeId = n.id;
            break;
        }
    }
    ASSERT_NE(cmdNodeId, InvalidNodeId);

    auto details = inspectConfigGraphNode(config, doc, cmdNodeId, &diag);
    ASSERT_TRUE(details.has_value());
    EXPECT_EQ(details->kind, NodeKind::Command);
    EXPECT_EQ(details->title, "FireWeapon");
    EXPECT_EQ(details->commandIndex, 0U);
    EXPECT_FALSE(details->connectedInputs.empty());
    EXPECT_FALSE(details->connectedControls.empty());
    EXPECT_FALSE(details->connectedGroups.empty());

    // Non-existent node returns nullopt
    auto invalidDetails = inspectConfigGraphNode(config, doc, 99999U, &diag);
    EXPECT_FALSE(invalidDetails.has_value());
}

TEST_F(ConfigGraphViewerTest, StateLifecycleAndSelection)
{
    auto config = createCleanConfig();
    ConfigGraphViewerState state;

    EXPECT_FALSE(state.isGraphSynchronized);
    EXPECT_EQ(state.getSelectedNodeId(), InvalidNodeId);
    EXPECT_FALSE(state.hasSelection());

    state.syncWithConfig(config);
    EXPECT_TRUE(state.isGraphSynchronized);
    EXPECT_GT(state.graphDocument.nodeCount(), 0U);
    EXPECT_FALSE(state.diagnosticsResult.hasErrors());

    // Selection
    const auto firstNodeId = state.graphDocument.nodes().front().id;
    state.selectNode(firstNodeId);
    EXPECT_TRUE(state.hasSelection());
    EXPECT_EQ(state.getSelectedNodeId(), firstNodeId);

    state.clearSelection();
    EXPECT_FALSE(state.hasSelection());
    EXPECT_EQ(state.getSelectedNodeId(), InvalidNodeId);
}

TEST_F(ConfigGraphViewerTest, FilterTogglesRebuildGraph)
{
    auto config = createCleanConfig();
    ConfigGraphViewerState state;
    state.syncWithConfig(config);

    const auto totalNodes = state.graphDocument.nodeCount();

    // Disable global settings
    state.showGlobalSettings = false;
    state.rebuildFromConfig(config);
    EXPECT_LT(state.graphDocument.nodeCount(), totalNodes);

    // Disable inputs
    state.showInputs = false;
    state.rebuildFromConfig(config);
    EXPECT_LT(state.graphDocument.nodeCount(), totalNodes);
}
