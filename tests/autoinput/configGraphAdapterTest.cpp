/**
 * @file configGraphAdapterTest.cpp
 * @brief Unit tests for ConfigData to GraphDocument conversion.
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>

#include "autoinput/config/config.h"
#include "autoinput_ui/graph/configGraphAdapter.h"
#include "autoinput_ui/graph/graphModel.h"

using namespace autoinput;
using namespace autoinput::ui::graph;

namespace
{
    [[nodiscard]] std::vector<const GraphNode*> getNodesOfKind(const GraphDocument& doc, NodeKind kind)
    {
        std::vector<const GraphNode*> result;
        for (const auto& node : doc.nodes())
        {
            if (node.kind == kind)
            {
                result.push_back(&node);
            }
        }
        return result;
    }
} // namespace

class ConfigGraphAdapterTest : public ::testing::Test
{
protected:
    ConfigData createSampleConfig()
    {
        ConfigData config;
        config.application = "notepad.exe";
        config.blacklist = { "game.exe", "overlay.exe" };
        config.endKey = "escape";

        CommandData cmd1;
        cmd1.name = "AutoFire";
        cmd1.action = "press";
        cmd1.startKeys = { "f1" };
        cmd1.keys = { "ctrl" };
        cmd1.buttons = { "left" };
        cmd1.exclusiveGroup = "Combat";
        cmd1.pressWait = "20ms";
        cmd1.releaseWait = "40ms";
        cmd1.controls = { CommandControlData{ .action = "pause", .input = "p" },
                          CommandControlData{ .action = "reload", .input = "r" } };

        CommandData cmd2;
        cmd2.name = "QuickHeal";
        cmd2.action = "press";
        cmd2.startKeys = { "f2" };
        cmd2.exclusiveGroup = "Combat"; // Shares group with cmd1

        config.commands = { cmd1, cmd2 };

        RecordedSequence seq1;
        seq1.name = "ComboSeq";
        seq1.start = "f3";
        seq1.repeat = true;
        seq1.events = { RecordedEvent{ .type = RecordedEventType::KeyDown, .delay = "10ms", .key = "a" },
                        RecordedEvent{ .type = RecordedEventType::KeyUp, .delay = "10ms", .key = "a" } };

        config.sequences = { seq1 };

        return config;
    }
};

TEST_F(ConfigGraphAdapterTest, ConvertEmptyConfig)
{
    ConfigData emptyConfig;
    ConfigGraphOptions opts;
    opts.includeGlobalSettings = true;

    auto doc = configToGraphDocument(emptyConfig, opts);
    EXPECT_EQ(doc.nodeCount(), 0U);
    EXPECT_EQ(doc.linkCount(), 0U);
}

TEST_F(ConfigGraphAdapterTest, ConvertConfigWithOneCommand)
{
    ConfigData config;
    CommandData cmd;
    cmd.name = "SingleCmd";
    cmd.action = "repeat";
    cmd.startKeys = { "f5" };
    cmd.keys = { "shift" };
    cmd.buttons = { "right" };
    cmd.pressWait = "10ms";
    cmd.releaseWait = "15ms";

    config.commands.push_back(cmd);

    ConfigGraphOptions opts;
    opts.includeGlobalSettings = false;

    auto doc = configToGraphDocument(config, opts);

    // Nodes:
    // 1 Command node
    // 3 Input nodes (1 startKey + 1 key + 1 button)
    EXPECT_EQ(doc.nodeCount(), 4U);

    auto cmdNodes = getNodesOfKind(doc, NodeKind::Command);
    ASSERT_EQ(cmdNodes.size(), 1U);
    EXPECT_EQ(cmdNodes[0]->title, "SingleCmd");
    EXPECT_EQ(cmdNodes[0]->sourceIndex, 0U);
    EXPECT_NE(cmdNodes[0]->subtitle.find("Action: repeat"), std::string::npos);

    auto inputNodes = getNodesOfKind(doc, NodeKind::Input);
    EXPECT_EQ(inputNodes.size(), 3U);

    // Links: 3 inputs -> command
    EXPECT_EQ(doc.linkCount(), 3U);
}

TEST_F(ConfigGraphAdapterTest, ConvertMultipleCommandsSharingExclusiveGroup)
{
    ConfigData config;

    CommandData cmd1;
    cmd1.name = "CmdA";
    cmd1.exclusiveGroup = "SharedGroup";

    CommandData cmd2;
    cmd2.name = "CmdB";
    cmd2.exclusiveGroup = "SharedGroup";

    config.commands = { cmd1, cmd2 };

    ConfigGraphOptions opts;
    opts.includeGlobalSettings = false;
    opts.deduplicateExclusiveGroups = true;

    auto doc = configToGraphDocument(config, opts);

    // Nodes: 2 commands + 1 shared exclusive group
    EXPECT_EQ(doc.nodeCount(), 3U);

    auto cmdNodes = getNodesOfKind(doc, NodeKind::Command);
    EXPECT_EQ(cmdNodes.size(), 2U);

    auto groupNodes = getNodesOfKind(doc, NodeKind::ExclusiveGroup);
    ASSERT_EQ(groupNodes.size(), 1U);
    EXPECT_EQ(groupNodes[0]->subtitle, "SharedGroup");

    // Links: 2 links (cmd1 -> group, cmd2 -> group)
    EXPECT_EQ(doc.linkCount(), 2U);
}

TEST_F(ConfigGraphAdapterTest, ConvertCommandWithControls)
{
    ConfigData config;

    CommandData cmd;
    cmd.name = "ControlledCmd";
    cmd.controls = { CommandControlData{ .action = "toggle", .input = "space" },
                     CommandControlData{ .action = "cancel", .input = "esc" } };

    config.commands.push_back(cmd);

    ConfigGraphOptions opts;
    opts.includeGlobalSettings = false;

    auto doc = configToGraphDocument(config, opts);

    // Nodes: 1 command + 2 controls
    EXPECT_EQ(doc.nodeCount(), 3U);

    auto ctrlNodes = getNodesOfKind(doc, NodeKind::Control);
    ASSERT_EQ(ctrlNodes.size(), 2U);
    EXPECT_EQ(ctrlNodes[0]->sourceIndex, 0U);
    EXPECT_EQ(ctrlNodes[1]->sourceIndex, 1U);
    EXPECT_EQ(ctrlNodes[0]->title, "Control: toggle");
    EXPECT_EQ(ctrlNodes[1]->title, "Control: cancel");

    // Links: 2 links (cmd -> control 0, cmd -> control 1)
    EXPECT_EQ(doc.linkCount(), 2U);
}

TEST_F(ConfigGraphAdapterTest, ConvertConfigWithSequences)
{
    ConfigData config;

    RecordedSequence seq1;
    seq1.name = "Macro1";
    seq1.start = "f9";
    seq1.repeat = false;

    RecordedSequence seq2;
    seq2.name = "Macro2";
    seq2.start = ""; // No start key

    config.sequences = { seq1, seq2 };

    ConfigGraphOptions opts;
    opts.includeGlobalSettings = false;

    auto doc = configToGraphDocument(config, opts);

    // Nodes: 2 sequences + 1 sequence start key (for seq1)
    EXPECT_EQ(doc.nodeCount(), 3U);

    auto seqNodes = getNodesOfKind(doc, NodeKind::Sequence);
    ASSERT_EQ(seqNodes.size(), 2U);
    EXPECT_EQ(seqNodes[0]->sourceIndex, 0U);
    EXPECT_EQ(seqNodes[1]->sourceIndex, 1U);
    EXPECT_EQ(seqNodes[0]->title, "Macro1");
    EXPECT_EQ(seqNodes[1]->title, "Macro2");

    // Links: 1 link (seq1 start key -> seq1)
    EXPECT_EQ(doc.linkCount(), 1U);
}

TEST_F(ConfigGraphAdapterTest, SourceIndicesPreservedAcrossElements)
{
    auto sampleConfig = createSampleConfig();
    ConfigGraphOptions opts;
    opts.includeGlobalSettings = false;

    auto doc = configToGraphDocument(sampleConfig, opts);

    auto cmdNodes = getNodesOfKind(doc, NodeKind::Command);
    ASSERT_EQ(cmdNodes.size(), 2U);
    EXPECT_EQ(cmdNodes[0]->sourceIndex, 0U);
    EXPECT_EQ(cmdNodes[1]->sourceIndex, 1U);

    auto ctrlNodes = getNodesOfKind(doc, NodeKind::Control);
    ASSERT_EQ(ctrlNodes.size(), 2U);
    EXPECT_EQ(ctrlNodes[0]->sourceIndex, 0U);
    EXPECT_EQ(ctrlNodes[1]->sourceIndex, 1U);

    auto seqNodes = getNodesOfKind(doc, NodeKind::Sequence);
    ASSERT_EQ(seqNodes.size(), 1U);
    EXPECT_EQ(seqNodes[0]->sourceIndex, 0U);
}

TEST_F(ConfigGraphAdapterTest, GlobalSettingsAndTargetLinks)
{
    auto sampleConfig = createSampleConfig();
    ConfigGraphOptions opts;
    opts.includeGlobalSettings = true;
    opts.linkGlobalSettingsToTargets = true;
    opts.deduplicateExclusiveGroups = true;

    auto doc = configToGraphDocument(sampleConfig, opts);

    // Verify global setting nodes exist
    auto appNodes = getNodesOfKind(doc, NodeKind::ApplicationFilter);
    ASSERT_EQ(appNodes.size(), 1U);
    EXPECT_EQ(appNodes[0]->subtitle, "notepad.exe");

    auto blNodes = getNodesOfKind(doc, NodeKind::BlacklistEntry);
    EXPECT_EQ(blNodes.size(), 2U);

    // Global settings: 1 AppFilter + 2 Blacklist + 1 EndKey = 4 global nodes
    // Each connects to 2 commands and 1 sequence = 4 * 3 = 12 global links
    // Commands & sequences specific links:
    // Cmd1: 1 startKey + 1 key + 1 button + 2 controls + 1 group = 6 links
    // Cmd2: 1 startKey + 1 group = 2 links
    // Seq1: 1 startKey = 1 link
    // Total links = 12 + 6 + 2 + 1 = 21 links
    EXPECT_EQ(doc.linkCount(), 21U);
}

TEST_F(ConfigGraphAdapterTest, DeterministicLayoutPositions)
{
    ConfigData config;

    CommandData cmd;
    cmd.name = "PositionTest";
    cmd.startKeys = { "f1" };
    cmd.controls = { CommandControlData{ .action = "action1", .input = "key1" } };
    config.commands.push_back(cmd);

    ConfigGraphOptions opts;
    opts.startX = 100.0F;
    opts.startY = 200.0F;
    opts.columnSpacing = 400.0F;
    opts.rowSpacing = 100.0F;
    opts.includeGlobalSettings = false;

    auto doc = configToGraphDocument(config, opts);

    auto inputNodes = getNodesOfKind(doc, NodeKind::Input);
    ASSERT_EQ(inputNodes.size(), 1U);
    EXPECT_FLOAT_EQ(inputNodes[0]->position.x, 100.0F);
    EXPECT_FLOAT_EQ(inputNodes[0]->position.y, 200.0F);

    auto cmdNodes = getNodesOfKind(doc, NodeKind::Command);
    ASSERT_EQ(cmdNodes.size(), 1U);
    EXPECT_FLOAT_EQ(cmdNodes[0]->position.x, 500.0F);
    EXPECT_FLOAT_EQ(cmdNodes[0]->position.y, 200.0F);

    auto ctrlNodes = getNodesOfKind(doc, NodeKind::Control);
    ASSERT_EQ(ctrlNodes.size(), 1U);
    EXPECT_FLOAT_EQ(ctrlNodes[0]->position.x, 900.0F);
    EXPECT_FLOAT_EQ(ctrlNodes[0]->position.y, 200.0F);
}

TEST_F(ConfigGraphAdapterTest, HelperFormatters)
{
    CommandData emptyCmd;
    EXPECT_EQ(formatCommandTitle(emptyCmd, 2), "Command 3");

    CommandData namedCmd{ .name = "MyCommand" };
    EXPECT_EQ(formatCommandTitle(namedCmd, 0), "MyCommand");

    CommandControlData emptyCtrl;
    EXPECT_EQ(formatControlTitle(emptyCtrl, 1), "Control 2");

    CommandControlData namedCtrl{ .action = "reload", .input = "r" };
    EXPECT_EQ(formatControlTitle(namedCtrl, 0), "Control: reload");
    EXPECT_NE(formatControlSubtitle(namedCtrl).find("Action: reload"), std::string::npos);

    RecordedSequence emptySeq;
    EXPECT_EQ(formatConfigSequenceTitle(emptySeq, 0), "Sequence 1");

    RecordedSequence namedSeq{ .name = "MySeq", .start = "f12", .repeat = true };
    EXPECT_EQ(formatConfigSequenceTitle(namedSeq, 0), "MySeq");
    EXPECT_NE(formatConfigSequenceSubtitle(namedSeq).find("Repeat: Yes"), std::string::npos);
    EXPECT_NE(formatConfigSequenceSubtitle(namedSeq).find("Start: f12"), std::string::npos);
}
