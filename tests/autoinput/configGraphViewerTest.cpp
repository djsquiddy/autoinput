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
        if (issue.category == "Input Conflict" &&
            issue.message.find("Duplicate command start input") != std::string::npos)
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
        if (issue.category == "Input Conflict" &&
            issue.message.find("Command and sequence start conflict") != std::string::npos)
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

// =========================================================================
// Safe Editing and Relationship Controller Tests
// =========================================================================

TEST_F(ConfigGraphViewerTest, BeginCommandEditAndDraftInitialization)
{
    auto config = createCleanConfig();
    ConfigGraphViewerState state;
    state.syncWithConfig(config);

    EXPECT_FALSE(state.editDraft.isActive);
    EXPECT_TRUE(state.beginCommandEdit(0, config));
    EXPECT_TRUE(state.editDraft.isActive);
    EXPECT_FALSE(state.editDraft.isDirty);
    EXPECT_EQ(state.editDraft.commandIndex, 0U);
    EXPECT_EQ(state.editDraft.name, "FireWeapon");
    EXPECT_EQ(state.editDraft.exclusiveGroup, "Combat");
    EXPECT_EQ(state.editDraft.startKeys.size(), 1U);
    EXPECT_EQ(state.editDraft.startKeys[0], "f1");
    EXPECT_EQ(state.editDraft.controls.size(), 1U);
    EXPECT_EQ(state.editDraft.controls[0].action, "pause");
    EXPECT_EQ(state.editDraft.controls[0].input, "p");
    EXPECT_FALSE(state.editDraft.hasErrors());

    // Out of bounds index fails gracefully
    EXPECT_FALSE(state.beginCommandEdit(999, config));
}

TEST_F(ConfigGraphViewerTest, CommandRenameAndApply)
{
    auto config = createCleanConfig();
    ConfigGraphViewerState state;
    state.syncWithConfig(config);

    ASSERT_TRUE(state.beginCommandEdit(0, config));
    state.setCommandName("PrimaryAttack");
    EXPECT_TRUE(state.editDraft.isDirty);
    EXPECT_TRUE(state.validateDraft(config));
    EXPECT_FALSE(state.editDraft.hasErrors());

    EXPECT_TRUE(state.applyCommandEdit(config));
    EXPECT_EQ(config.commands[0].name, "PrimaryAttack");
    EXPECT_FALSE(state.editDraft.isDirty);
    EXPECT_TRUE(state.isGraphSynchronized);

    // Verify graph document has updated node title
    bool foundUpdatedNode = false;
    for (const auto& node : state.graphDocument.nodes())
    {
        if (node.kind == NodeKind::Command && node.sourceIndex == 0U)
        {
            EXPECT_EQ(node.title, "PrimaryAttack");
            foundUpdatedNode = true;
            break;
        }
    }
    EXPECT_TRUE(foundUpdatedNode);
}

TEST_F(ConfigGraphViewerTest, ExclusiveGroupAndStartKeysEdit)
{
    auto config = createCleanConfig();
    ConfigGraphViewerState state;
    state.syncWithConfig(config);

    ASSERT_TRUE(state.beginCommandEdit(0, config));
    state.setExclusiveGroup("WeaponsGroup");
    state.addStartKey("f5");
    EXPECT_EQ(state.editDraft.startKeys.size(), 2U);
    EXPECT_TRUE(state.setStartKey(0, "f6"));
    EXPECT_TRUE(state.removeStartKey(1));
    EXPECT_EQ(state.editDraft.startKeys.size(), 1U);
    EXPECT_EQ(state.editDraft.startKeys[0], "f6");

    EXPECT_TRUE(state.applyCommandEdit(config));
    EXPECT_EQ(config.commands[0].exclusiveGroup, "WeaponsGroup");
    ASSERT_EQ(config.commands[0].startKeys.size(), 1U);
    EXPECT_EQ(config.commands[0].startKeys[0], "f6");
}

TEST_F(ConfigGraphViewerTest, CommandControlAddAndRemove)
{
    auto config = createCleanConfig();
    ConfigGraphViewerState state;
    state.syncWithConfig(config);

    ASSERT_TRUE(state.beginCommandEdit(0, config));
    EXPECT_EQ(state.editDraft.controls.size(), 1U);

    // Add control
    state.addControl("toggle", "t");
    EXPECT_EQ(state.editDraft.controls.size(), 2U);
    EXPECT_TRUE(state.applyCommandEdit(config));
    ASSERT_EQ(config.commands[0].controls.size(), 2U);
    EXPECT_EQ(config.commands[0].controls[1].action, "toggle");
    EXPECT_EQ(config.commands[0].controls[1].input, "t");

    // Remove first control
    ASSERT_TRUE(state.beginCommandEdit(0, config));
    EXPECT_TRUE(state.removeControl(0));
    EXPECT_EQ(state.editDraft.controls.size(), 1U);
    EXPECT_TRUE(state.applyCommandEdit(config));
    ASSERT_EQ(config.commands[0].controls.size(), 1U);
    EXPECT_EQ(config.commands[0].controls[0].action, "toggle");
    EXPECT_EQ(config.commands[0].controls[0].input, "t");
}

TEST_F(ConfigGraphViewerTest, UpdateControlActionAndInput)
{
    auto config = createCleanConfig();
    ConfigGraphViewerState state;
    state.syncWithConfig(config);

    ASSERT_TRUE(state.beginCommandEdit(0, config));
    EXPECT_TRUE(state.updateControl(0, "resume", "r"));
    EXPECT_TRUE(state.applyCommandEdit(config));
    ASSERT_EQ(config.commands[0].controls.size(), 1U);
    EXPECT_EQ(config.commands[0].controls[0].action, "resume");
    EXPECT_EQ(config.commands[0].controls[0].input, "r");

    // Invalid control index returns false
    EXPECT_FALSE(state.updateControl(999, "stop", "s"));
}

TEST_F(ConfigGraphViewerTest, RejectEmptyCommandNameDoesNotMutateOriginalConfig)
{
    auto config = createCleanConfig();
    const auto originalConfig = config;

    ConfigGraphViewerState state;
    state.syncWithConfig(config);

    ASSERT_TRUE(state.beginCommandEdit(0, config));
    state.setCommandName("   ");
    EXPECT_FALSE(state.validateDraft(config));
    EXPECT_TRUE(state.editDraft.hasErrors());

    // Attempting to apply invalid draft must fail and preserve target config
    EXPECT_FALSE(state.applyCommandEdit(config));
    EXPECT_EQ(config.commands[0].name, originalConfig.commands[0].name);
}

TEST_F(ConfigGraphViewerTest, RejectInvalidControlActionDoesNotMutateOriginalConfig)
{
    auto config = createCleanConfig();
    const auto originalConfig = config;

    ConfigGraphViewerState state;
    state.syncWithConfig(config);

    ASSERT_TRUE(state.beginCommandEdit(0, config));
    state.addControl("non_existent_control_action", "ctrl+x");
    EXPECT_FALSE(state.validateDraft(config));
    EXPECT_TRUE(state.editDraft.hasErrors());

    // Attempting to apply invalid action must fail and leave target config untouched
    EXPECT_FALSE(state.applyCommandEdit(config));
    EXPECT_EQ(config.commands[0].controls.size(), originalConfig.commands[0].controls.size());
    EXPECT_EQ(config.commands[0].controls[0].action, originalConfig.commands[0].controls[0].action);
}

TEST_F(ConfigGraphViewerTest, CancelDraftRevertsChangesWithoutMutating)
{
    auto config = createCleanConfig();
    const auto originalConfig = config;

    ConfigGraphViewerState state;
    state.syncWithConfig(config);

    ASSERT_TRUE(state.beginCommandEdit(0, config));
    state.setCommandName("UnsavedRename");
    state.setExclusiveGroup("TemporaryGroup");
    state.addStartKey("f12");
    state.addControl("stop", "q");
    EXPECT_TRUE(state.editDraft.isDirty);

    // Cancel draft
    state.cancelCommandEdit();
    EXPECT_FALSE(state.editDraft.isActive);
    EXPECT_FALSE(state.editDraft.isDirty);

    // Config must remain identical to original
    EXPECT_EQ(config.commands[0].name, originalConfig.commands[0].name);
    EXPECT_EQ(config.commands[0].exclusiveGroup, originalConfig.commands[0].exclusiveGroup);
    EXPECT_EQ(config.commands[0].startKeys, originalConfig.commands[0].startKeys);
    EXPECT_EQ(config.commands[0].controls.size(), originalConfig.commands[0].controls.size());
}
