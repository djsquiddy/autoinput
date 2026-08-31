/**
 * @file graphLayoutMetadataTest.cpp
 * @brief Unit tests for graph layout metadata persistence, serialization, and stale cleanup.
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput_ui/graph/graphLayoutMetadata.h"
#include "autoinput/config/config.h"
#include "autoinput_ui/graph/graphModel.h"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

using namespace autoinput::ui::graph;

class GraphLayoutMetadataTest : public ::testing::Test
{
protected:
    std::filesystem::path m_tempDir;

    void SetUp() override
    {
        m_tempDir = std::filesystem::temp_directory_path() / "autoinput_layout_test";
        std::filesystem::create_directories(m_tempDir);
    }

    void TearDown() override
    {
        std::error_code ec;
        std::filesystem::remove_all(m_tempDir, ec);
    }
};

// =========================================================================
// Stable Key Generation Tests
// =========================================================================

TEST_F(GraphLayoutMetadataTest, StableKeyGenerationAcrossNodeKinds)
{
    GraphNode startNode{ .id = 1, .kind = NodeKind::Start, .title = "Start" };
    EXPECT_EQ(getNodeStableKey(startNode), "start");

    GraphNode endNode{ .id = 2, .kind = NodeKind::End, .title = "End" };
    EXPECT_EQ(getNodeStableKey(endNode), "end");

    GraphNode globalEndNode{ .id = 3, .kind = NodeKind::End, .title = "Global End" };
    EXPECT_EQ(getNodeStableKey(globalEndNode), "global:end");

    GraphNode eventNode{ .id = 4, .kind = NodeKind::RecordedEvent, .title = "Key A", .sourceIndex = 3 };
    EXPECT_EQ(getNodeStableKey(eventNode), "event:3");

    GraphNode waitNode{ .id = 5, .kind = NodeKind::Wait, .title = "Wait 100ms", .sourceIndex = 3 };
    EXPECT_EQ(getNodeStableKey(waitNode), "wait:3");

    GraphNode cmdNode{ .id = 6, .kind = NodeKind::Command, .title = "AutoClicker" };
    EXPECT_EQ(getNodeStableKey(cmdNode), "cmd:AutoClicker");

    GraphNode ctrlNode{ .id = 7, .kind = NodeKind::Control, .title = "AutoClicker", .sourceIndex = 0 };
    EXPECT_EQ(getNodeStableKey(ctrlNode), "ctrl:AutoClicker:0");

    GraphNode seqNode{ .id = 8, .kind = NodeKind::Sequence, .title = "ComboMacro" };
    EXPECT_EQ(getNodeStableKey(seqNode), "seq:ComboMacro");

    GraphNode inputNode{ .id = 9, .kind = NodeKind::Input, .title = "F1" };
    EXPECT_EQ(getNodeStableKey(inputNode), "input:F1");

    GraphNode groupNode{ .id = 10, .kind = NodeKind::ExclusiveGroup, .title = "CombatGroup" };
    EXPECT_EQ(getNodeStableKey(groupNode), "group:CombatGroup");

    GraphNode filterNode{ .id = 11, .kind = NodeKind::ApplicationFilter, .title = "App Filter" };
    EXPECT_EQ(getNodeStableKey(filterNode), "filter:app");

    GraphNode blacklistNode{ .id = 12, .kind = NodeKind::BlacklistEntry, .title = "calc.exe", .sourceIndex = 1 };
    EXPECT_EQ(getNodeStableKey(blacklistNode), "blacklist:1");

    GraphNode commentNode{ .id = 13, .kind = NodeKind::Comment, .title = "Note" };
    EXPECT_EQ(getNodeStableKey(commentNode), "comment:13");
}

// =========================================================================
// Layout Application and Extraction Tests
// =========================================================================

TEST_F(GraphLayoutMetadataTest, ExtractAndApplyLayoutToGraph)
{
    GraphDocument graph;
    auto& start = graph.createNode(NodeKind::Start, "Start", { 10.0F, 20.0F });
    auto& event0 = graph.createNode(NodeKind::RecordedEvent, "Key Space", { 100.0F, 50.0F });
    event0.sourceIndex = 0;
    auto& end = graph.createNode(NodeKind::End, "End", { 300.0F, 50.0F });

    GraphLayoutViewSettings viewSettings{ .offset = { 5.0F, 15.0F }, .zoom = 1.25F };
    auto layout = extractLayoutFromGraph(graph, viewSettings, "TestSequence");

    EXPECT_EQ(layout.graphName, "TestSequence");
    EXPECT_FLOAT_EQ(layout.viewSettings.zoom, 1.25F);
    EXPECT_FLOAT_EQ(layout.viewSettings.offset.x, 5.0F);
    EXPECT_FLOAT_EQ(layout.viewSettings.offset.y, 15.0F);
    EXPECT_EQ(layout.nodeCount(), 3U);

    ASSERT_NE(layout.findNodeLayout("start"), nullptr);
    EXPECT_FLOAT_EQ(layout.findNodeLayout("start")->position.x, 10.0F);
    EXPECT_FLOAT_EQ(layout.findNodeLayout("start")->position.y, 20.0F);

    ASSERT_NE(layout.findNodeLayout("event:0"), nullptr);
    EXPECT_FLOAT_EQ(layout.findNodeLayout("event:0")->position.x, 100.0F);
    EXPECT_FLOAT_EQ(layout.findNodeLayout("event:0")->position.y, 50.0F);

    // Modify layout positions
    layout.setNodeLayout("start", { 40.0F, 60.0F });
    layout.setNodeLayout("event:0", { 180.0F, 120.0F });

    // Apply back onto graph
    const std::size_t applied = applyLayoutToGraph(graph, layout);
    EXPECT_EQ(applied, 3U); // start, event:0, end

    const auto* updatedStart = graph.findNode(start.id);
    ASSERT_NE(updatedStart, nullptr);
    EXPECT_FLOAT_EQ(updatedStart->position.x, 40.0F);
    EXPECT_FLOAT_EQ(updatedStart->position.y, 60.0F);

    const auto* updatedEvent0 = graph.findNode(event0.id);
    ASSERT_NE(updatedEvent0, nullptr);
    EXPECT_FLOAT_EQ(updatedEvent0->position.x, 180.0F);
    EXPECT_FLOAT_EQ(updatedEvent0->position.y, 120.0F);
}

// =========================================================================
// Serialization Roundtrip Tests
// =========================================================================

TEST_F(GraphLayoutMetadataTest, SingleGraphLayoutTomlRoundtrip)
{
    GraphLayoutDocument original;
    original.version = 1;
    original.graphName = "CustomGraph";
    original.viewSettings.offset = { 25.0F, -50.0F };
    original.viewSettings.zoom = 1.5F;
    original.viewSettings.showCommands = true;
    original.viewSettings.showBlacklist = false;

    original.setNodeLayout("start", { 100.0F, 200.0F }, false);
    original.setNodeLayout("cmd:Attack", { 300.0F, 250.0F }, true);
    auto* attackLayout = original.findNodeLayout("cmd:Attack");
    ASSERT_NE(attackLayout, nullptr);
    attackLayout->comment = "Main attack action";

    const std::string tomlStr = serializeGraphLayoutToToml(original);
    EXPECT_FALSE(tomlStr.empty());

    std::string errorMsg;
    const auto deserialized = deserializeGraphLayoutFromToml(tomlStr, &errorMsg);
    ASSERT_TRUE(deserialized.has_value()) << "Error: " << errorMsg;

    EXPECT_EQ(deserialized->version, 1U);
    EXPECT_EQ(deserialized->graphName, "CustomGraph");
    EXPECT_FLOAT_EQ(deserialized->viewSettings.offset.x, 25.0F);
    EXPECT_FLOAT_EQ(deserialized->viewSettings.offset.y, -50.0F);
    EXPECT_FLOAT_EQ(deserialized->viewSettings.zoom, 1.5F);
    EXPECT_TRUE(deserialized->viewSettings.showCommands);
    EXPECT_FALSE(deserialized->viewSettings.showBlacklist);

    const auto* startNode = deserialized->findNodeLayout("start");
    ASSERT_NE(startNode, nullptr);
    EXPECT_FLOAT_EQ(startNode->position.x, 100.0F);
    EXPECT_FLOAT_EQ(startNode->position.y, 200.0F);
    EXPECT_FALSE(startNode->collapsed);

    const auto* cmdNode = deserialized->findNodeLayout("cmd:Attack");
    ASSERT_NE(cmdNode, nullptr);
    EXPECT_FLOAT_EQ(cmdNode->position.x, 300.0F);
    EXPECT_FLOAT_EQ(cmdNode->position.y, 250.0F);
    EXPECT_TRUE(cmdNode->collapsed);
    ASSERT_TRUE(cmdNode->comment.has_value());
    EXPECT_EQ(*cmdNode->comment, "Main attack action");
}

TEST_F(GraphLayoutMetadataTest, ConfigGraphMetadataDocumentTomlRoundtrip)
{
    ConfigGraphMetadataDocument doc;
    doc.version = 1;
    doc.format = "autoinput-graph-layout";

    // Top-level config graph layout
    doc.configGraphLayout.graphName = "config";
    doc.configGraphLayout.viewSettings.zoom = 1.1F;
    doc.configGraphLayout.setNodeLayout("cmd:Heal", { 100.0F, 100.0F });
    doc.configGraphLayout.setNodeLayout("input:F5", { 50.0F, 100.0F });

    // Sequence graph layout
    auto& seqLayout = doc.getOrCreateSequenceLayout("SpeedRun");
    seqLayout.viewSettings.offset = { 0.0F, 0.0F };
    seqLayout.viewSettings.zoom = 2.0F;
    seqLayout.setNodeLayout("start", { 0.0F, 50.0F });
    seqLayout.setNodeLayout("event:0", { 150.0F, 50.0F });
    seqLayout.setNodeLayout("end", { 300.0F, 50.0F });

    const std::string tomlStr = serializeGraphMetadataToToml(doc);
    EXPECT_FALSE(tomlStr.empty());

    std::string errorMsg;
    const auto loadedDoc = deserializeGraphMetadataFromToml(tomlStr, &errorMsg);
    ASSERT_TRUE(loadedDoc.has_value()) << "Error: " << errorMsg;

    EXPECT_EQ(loadedDoc->version, 1U);
    EXPECT_EQ(loadedDoc->format, "autoinput-graph-layout");

    // Verify config graph
    EXPECT_FLOAT_EQ(loadedDoc->configGraphLayout.viewSettings.zoom, 1.1F);
    ASSERT_NE(loadedDoc->configGraphLayout.findNodeLayout("cmd:Heal"), nullptr);
    EXPECT_FLOAT_EQ(loadedDoc->configGraphLayout.findNodeLayout("cmd:Heal")->position.x, 100.0F);

    // Verify sequence graph
    const auto* loadedSeq = loadedDoc->findSequenceLayout("SpeedRun");
    ASSERT_NE(loadedSeq, nullptr);
    EXPECT_FLOAT_EQ(loadedSeq->viewSettings.zoom, 2.0F);
    ASSERT_NE(loadedSeq->findNodeLayout("event:0"), nullptr);
    EXPECT_FLOAT_EQ(loadedSeq->findNodeLayout("event:0")->position.x, 150.0F);
}

// =========================================================================
// File Persistence and Sidecar Path Tests
// =========================================================================

TEST_F(GraphLayoutMetadataTest, SidecarPathGeneration)
{
    const std::filesystem::path cfgPath = "configs/gaming/macro.toml";
    const auto metaPath = getGraphMetadataPathForConfig(cfgPath);
    EXPECT_EQ(metaPath.filename().string(), "macro.graph.toml");
    EXPECT_EQ(metaPath.parent_path().string(), "configs/gaming");

    // Already ends with .graph.toml
    const std::filesystem::path existingSidecar = "configs/test.graph.toml";
    EXPECT_EQ(getGraphMetadataPathForConfig(existingSidecar), existingSidecar);

    // Empty path handling
    EXPECT_TRUE(getGraphMetadataPathForConfig({}).empty());
}

TEST_F(GraphLayoutMetadataTest, SaveAndLoadMetadataFile)
{
    const auto metaFilePath = m_tempDir / "sample_profile.graph.toml";
    const auto configFilePath = m_tempDir / "sample_profile.toml";

    ConfigGraphMetadataDocument doc;
    doc.configGraphLayout.setNodeLayout("cmd:Jump", { 120.0F, 340.0F });

    // Save metadata
    EXPECT_TRUE(saveGraphMetadataFile(metaFilePath, doc));
    EXPECT_TRUE(std::filesystem::exists(metaFilePath));

    // Load by metadata path
    const auto loaded1 = loadGraphMetadataFile(metaFilePath);
    ASSERT_TRUE(loaded1.has_value());
    ASSERT_NE(loaded1->configGraphLayout.findNodeLayout("cmd:Jump"), nullptr);
    EXPECT_FLOAT_EQ(loaded1->configGraphLayout.findNodeLayout("cmd:Jump")->position.x, 120.0F);

    // Load using config path
    const auto loaded2 = loadGraphMetadataForConfig(configFilePath);
    ASSERT_TRUE(loaded2.has_value());
    ASSERT_NE(loaded2->configGraphLayout.findNodeLayout("cmd:Jump"), nullptr);
    EXPECT_FLOAT_EQ(loaded2->configGraphLayout.findNodeLayout("cmd:Jump")->position.y, 340.0F);
}

// =========================================================================
// Missing and Invalid Metadata Handling Tests
// =========================================================================

TEST_F(GraphLayoutMetadataTest, MissingMetadataReturnsNulloptGracefully)
{
    const auto missingPath = m_tempDir / "non_existent_config.toml";
    const auto result = loadGraphMetadataForConfig(missingPath);
    EXPECT_FALSE(result.has_value());

    const auto directMissing = loadGraphMetadataFile(missingPath);
    EXPECT_FALSE(directMissing.has_value());
}

TEST_F(GraphLayoutMetadataTest, CorruptedTomlReturnsNulloptGracefully)
{
    const auto corruptedPath = m_tempDir / "corrupted.graph.toml";
    {
        std::ofstream out(corruptedPath);
        out << "invalid_toml = [ this is not closed table {";
    }

    std::string errorMsg;
    const auto result = deserializeGraphMetadataFromToml("invalid_toml = {", &errorMsg);
    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(errorMsg.empty());

    const auto fileResult = loadGraphMetadataFile(corruptedPath);
    EXPECT_FALSE(fileResult.has_value());
}

TEST_F(GraphLayoutMetadataTest, VersionMismatchDefaultAndGracefulHandling)
{
    // A file with a newer future version e.g. 99
    const std::string futureVersionToml = R"(
[metadata]
version = 99
format = "autoinput-graph-layout"

[config_graph]
[config_graph.view]
zoom = 2.5

[config_graph.nodes]
"cmd:FutureCmd" = { x = 500.0, y = 600.0 }
)";

    const auto parsed = deserializeGraphMetadataFromToml(futureVersionToml);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->version, 99U);
    EXPECT_FLOAT_EQ(parsed->configGraphLayout.viewSettings.zoom, 2.5F);
    ASSERT_NE(parsed->configGraphLayout.findNodeLayout("cmd:FutureCmd"), nullptr);
}

// =========================================================================
// Stale Metadata Cleanup Tests
// =========================================================================

TEST_F(GraphLayoutMetadataTest, PruneStaleSequenceEventMetadata)
{
    GraphLayoutDocument seqLayout;
    seqLayout.setNodeLayout("start", { 0.0F, 0.0F });
    seqLayout.setNodeLayout("event:0", { 100.0F, 0.0F });
    seqLayout.setNodeLayout("event:1", { 200.0F, 0.0F });
    seqLayout.setNodeLayout("event:2", { 300.0F, 0.0F });
    seqLayout.setNodeLayout("event:3", { 400.0F, 0.0F }); // Stale!
    seqLayout.setNodeLayout("end", { 500.0F, 0.0F });

    // Sequence with only 3 events (indices 0, 1, 2)
    autoinput::RecordedSequence seq;
    seq.name = "MySeq";
    seq.events.resize(3);

    const std::size_t pruned = pruneStaleMetadata(seqLayout, seq);
    EXPECT_EQ(pruned, 1U);
    EXPECT_EQ(seqLayout.nodeCount(), 5U);
    EXPECT_EQ(seqLayout.findNodeLayout("event:3"), nullptr);
    EXPECT_NE(seqLayout.findNodeLayout("event:2"), nullptr);
}

TEST_F(GraphLayoutMetadataTest, PruneStaleConfigAndSequenceLayouts)
{
    ConfigGraphMetadataDocument doc;

    // Config graph layout with 3 commands
    doc.configGraphLayout.setNodeLayout("cmd:KeepMe", { 10.0F, 10.0F });
    doc.configGraphLayout.setNodeLayout("cmd:DeleteMe", { 20.0F, 20.0F }); // Stale
    doc.configGraphLayout.setNodeLayout("input:F1", { 30.0F, 30.0F });
    doc.configGraphLayout.setNodeLayout("global:end", { 40.0F, 40.0F });

    // Sequence layouts
    auto& activeSeqLayout = doc.getOrCreateSequenceLayout("ActiveSeq");
    activeSeqLayout.setNodeLayout("start", { 0.0F, 0.0F });
    auto& deletedSeqLayout = doc.getOrCreateSequenceLayout("DeletedSeq"); // Stale
    deletedSeqLayout.setNodeLayout("start", { 0.0F, 0.0F });

    // Actual config
    autoinput::ConfigData config;
    autoinput::CommandData cmd;
    cmd.name = "KeepMe";
    cmd.keys.push_back("F1");
    config.commands.push_back(cmd);

    autoinput::RecordedSequence activeSeq;
    activeSeq.name = "ActiveSeq";
    config.sequences.push_back(activeSeq);

    const std::size_t totalPruned = pruneStaleMetadata(doc, config);
    EXPECT_EQ(totalPruned, 2U); // 1 stale command node + 1 deleted sequence layout

    EXPECT_NE(doc.configGraphLayout.findNodeLayout("cmd:KeepMe"), nullptr);
    EXPECT_EQ(doc.configGraphLayout.findNodeLayout("cmd:DeleteMe"), nullptr);
    EXPECT_NE(doc.findSequenceLayout("ActiveSeq"), nullptr);
    EXPECT_EQ(doc.findSequenceLayout("DeletedSeq"), nullptr);
}

TEST_F(GraphLayoutMetadataTest, PruneStaleNodesFromArbitraryGraphDocument)
{
    GraphLayoutDocument layout;
    layout.setNodeLayout("start", { 0.0F, 0.0F });
    layout.setNodeLayout("cmd:OldCommand", { 50.0F, 50.0F });
    layout.setNodeLayout("cmd:NewCommand", { 100.0F, 100.0F });

    GraphDocument graph;
    graph.createNode(NodeKind::Start, "Start");
    graph.createNode(NodeKind::Command, "NewCommand");

    const std::size_t pruned = pruneStaleMetadata(layout, graph);
    EXPECT_EQ(pruned, 1U);
    EXPECT_EQ(layout.nodeCount(), 2U);
    EXPECT_EQ(layout.findNodeLayout("cmd:OldCommand"), nullptr);
    EXPECT_NE(layout.findNodeLayout("cmd:NewCommand"), nullptr);
}
