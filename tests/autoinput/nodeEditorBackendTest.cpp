/**
 * @file nodeEditorBackendTest.cpp
 * @brief Unit tests for the node editor backend abstraction and selection logic.
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput_ui/graph/nodeEditorBackend.h"
#include <gtest/gtest.h>

using namespace autoinput::ui::graph;

class NodeEditorBackendTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(NodeEditorBackendTest, BackendTypeToStringConversions)
{
    EXPECT_EQ(backendTypeToString(NodeEditorBackendType::Fallback), "Fallback");
    EXPECT_EQ(backendTypeToString(NodeEditorBackendType::ImNodes), "ImNodes");
    EXPECT_EQ(backendTypeToString(NodeEditorBackendType::ImguiNodeEditor), "ImguiNodeEditor");
    EXPECT_EQ(backendTypeToString(NodeEditorBackendType::CustomCanvas), "CustomCanvas");
    EXPECT_EQ(backendTypeToString(static_cast<NodeEditorBackendType>(99)), "Unknown");
}

TEST_F(NodeEditorBackendTest, BackendAvailabilityCheck)
{
    EXPECT_TRUE(isBackendAvailable(NodeEditorBackendType::Fallback));
    EXPECT_TRUE(isBackendAvailable(NodeEditorBackendType::CustomCanvas));

#ifdef AUTOINPUT_HAS_IMNODES
    EXPECT_TRUE(isBackendAvailable(NodeEditorBackendType::ImNodes));
#else
    EXPECT_FALSE(isBackendAvailable(NodeEditorBackendType::ImNodes));
#endif

#ifdef AUTOINPUT_HAS_IMGUI_NODE_EDITOR
    EXPECT_TRUE(isBackendAvailable(NodeEditorBackendType::ImguiNodeEditor));
#else
    EXPECT_FALSE(isBackendAvailable(NodeEditorBackendType::ImguiNodeEditor));
#endif

    EXPECT_FALSE(isBackendAvailable(static_cast<NodeEditorBackendType>(99)));
}

TEST_F(NodeEditorBackendTest, PreferredBackendSelection)
{
    resetPreferredBackendType();
    const auto preferred = getPreferredBackendType();

#if defined(AUTOINPUT_HAS_IMGUI_NODE_EDITOR)
    EXPECT_EQ(preferred, NodeEditorBackendType::ImguiNodeEditor);
#elif defined(AUTOINPUT_HAS_IMNODES)
    EXPECT_EQ(preferred, NodeEditorBackendType::ImNodes);
#else
    EXPECT_EQ(preferred, NodeEditorBackendType::Fallback);
#endif

    EXPECT_TRUE(isBackendAvailable(preferred));
}

TEST_F(NodeEditorBackendTest, SetAndResetPreferredBackendType)
{
    resetPreferredBackendType();
    const auto defaultPreferred = getPreferredBackendType();

    // Fallback is always available
    setPreferredBackendType(NodeEditorBackendType::Fallback);
    EXPECT_EQ(getPreferredBackendType(), NodeEditorBackendType::Fallback);

    // CustomCanvas is always available
    setPreferredBackendType(NodeEditorBackendType::CustomCanvas);
    EXPECT_EQ(getPreferredBackendType(), NodeEditorBackendType::CustomCanvas);

    // Resetting restores default automatic priority
    resetPreferredBackendType();
    EXPECT_EQ(getPreferredBackendType(), defaultPreferred);

    // Setting an unavailable backend type is ignored and does not change preference
    setPreferredBackendType(static_cast<NodeEditorBackendType>(99));
    EXPECT_EQ(getPreferredBackendType(), defaultPreferred);
}

TEST_F(NodeEditorBackendTest, CreateDefaultBackend)
{
    auto backend = createDefaultNodeEditorBackend();
    ASSERT_NE(backend, nullptr);
    EXPECT_EQ(backend->backendType(), getPreferredBackendType());
    EXPECT_TRUE(backend->capabilities().isAvailable);
}

TEST_F(NodeEditorBackendTest, CreateFallbackBackendExplicitly)
{
    auto backend = createNodeEditorBackend(NodeEditorBackendType::Fallback);
    ASSERT_NE(backend, nullptr);
    EXPECT_EQ(backend->backendType(), NodeEditorBackendType::Fallback);

    const auto& caps = backend->capabilities();
    EXPECT_TRUE(caps.isAvailable);
    EXPECT_FALSE(caps.supportsCanvas);
    EXPECT_FALSE(caps.supportsPositions);
    EXPECT_FALSE(caps.supportsLinkCreationQuery);
    EXPECT_FALSE(caps.supportsLinkDeletionQuery);
    EXPECT_FALSE(caps.supportsSelectionQuery);
    EXPECT_FALSE(caps.supportsGroups);
    EXPECT_FALSE(caps.supportsComments);
    EXPECT_FALSE(caps.supportsMinimap);
    EXPECT_FALSE(caps.supportsZoom);
    EXPECT_FALSE(caps.supportsMultiSelect);
    EXPECT_EQ(caps.backendName, "Fallback");
    EXPECT_FALSE(caps.description.empty());
}

TEST_F(NodeEditorBackendTest, FallbackBackendSafeLifecycleAndCalls)
{
    FallbackNodeEditorBackend fallback;

    // Lifecycle
    fallback.initialize();
    fallback.shutdown();

    // Canvas & nodes
    fallback.beginCanvas("TestCanvas");
    fallback.beginNode(1);
    fallback.beginNodeTitle();
    fallback.endNodeTitle();
    fallback.beginInputPin(10);
    fallback.endInputPin();
    fallback.beginOutputPin(20);
    fallback.endOutputPin();
    fallback.endNode();

    // Links
    fallback.drawLink(100, 20, 10);
    fallback.endCanvas();

    // Queries
    EXPECT_FALSE(fallback.queryCreatedLink().has_value());
    EXPECT_FALSE(fallback.queryDeletedLink().has_value());
    EXPECT_TRUE(fallback.querySelectedNodes().empty());
    EXPECT_TRUE(fallback.querySelectedLinks().empty());

    // Positions
    fallback.setNodePosition(1, { .x = 150.0F, .y = 250.0F });
    EXPECT_FALSE(fallback.getNodePosition(1).has_value());
}

TEST_F(NodeEditorBackendTest, CreateInvalidOrDisabledBackend)
{
    auto invalidBackend = createNodeEditorBackend(static_cast<NodeEditorBackendType>(99));
    EXPECT_EQ(invalidBackend, nullptr);

#ifndef AUTOINPUT_HAS_IMNODES
    auto imnodesBackend = createNodeEditorBackend(NodeEditorBackendType::ImNodes);
    EXPECT_EQ(imnodesBackend, nullptr);
#endif

#ifndef AUTOINPUT_HAS_IMGUI_NODE_EDITOR
    auto imguiNodeEditorBackend = createNodeEditorBackend(NodeEditorBackendType::ImguiNodeEditor);
    EXPECT_EQ(imguiNodeEditorBackend, nullptr);
#endif
}

#ifdef AUTOINPUT_HAS_IMNODES
TEST_F(NodeEditorBackendTest, ImnodesBackendCapabilitiesWhenEnabled)
{
    auto backend = createNodeEditorBackend(NodeEditorBackendType::ImNodes);
    ASSERT_NE(backend, nullptr);
    EXPECT_EQ(backend->backendType(), NodeEditorBackendType::ImNodes);

    const auto& caps = backend->capabilities();
    EXPECT_TRUE(caps.isAvailable);
    EXPECT_TRUE(caps.supportsCanvas);
    EXPECT_TRUE(caps.supportsPositions);
    EXPECT_TRUE(caps.supportsLinkCreationQuery);
    EXPECT_TRUE(caps.supportsLinkDeletionQuery);
    EXPECT_TRUE(caps.supportsSelectionQuery);
    EXPECT_FALSE(caps.supportsGroups);
    EXPECT_FALSE(caps.supportsComments);
    EXPECT_FALSE(caps.supportsMinimap);
    EXPECT_FALSE(caps.supportsZoom);
    EXPECT_TRUE(caps.supportsMultiSelect);
    EXPECT_EQ(caps.backendName, "imnodes");
}
#endif

#ifdef AUTOINPUT_HAS_IMGUI_NODE_EDITOR
TEST_F(NodeEditorBackendTest, ImguiNodeEditorBackendCapabilitiesWhenEnabled)
{
    auto backend = createNodeEditorBackend(NodeEditorBackendType::ImguiNodeEditor);
    ASSERT_NE(backend, nullptr);
    EXPECT_EQ(backend->backendType(), NodeEditorBackendType::ImguiNodeEditor);

    const auto& caps = backend->capabilities();
    EXPECT_TRUE(caps.isAvailable);
    EXPECT_TRUE(caps.supportsCanvas);
    EXPECT_TRUE(caps.supportsPositions);
    EXPECT_TRUE(caps.supportsLinkCreationQuery);
    EXPECT_TRUE(caps.supportsLinkDeletionQuery);
    EXPECT_TRUE(caps.supportsSelectionQuery);
    EXPECT_TRUE(caps.supportsGroups);
    EXPECT_TRUE(caps.supportsComments);
    EXPECT_FALSE(caps.supportsMinimap);
    EXPECT_TRUE(caps.supportsZoom);
    EXPECT_TRUE(caps.supportsMultiSelect);
    EXPECT_EQ(caps.backendName, "imgui-node-editor");
}
#endif
