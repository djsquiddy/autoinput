/**
 * @file nodeEditorBackend.cpp
 * @brief Node editor backend abstraction and fallback implementation.
 * @author djsquiddy
 * @date August 2026
 */
#include "nodeEditorBackend.h"
#include "customCanvasBackend.h"

#ifdef AUTOINPUT_HAS_IMNODES
#include "imnodesBackend.h"
#endif

#ifdef AUTOINPUT_HAS_IMGUI_NODE_EDITOR
#include "imguiNodeEditorBackend.h"
#endif

namespace autoinput::ui::graph
{
    FallbackNodeEditorBackend::FallbackNodeEditorBackend()
    {
        m_capabilities.isAvailable = true;
        m_capabilities.supportsCanvas = false;
        m_capabilities.supportsPositions = false;
        m_capabilities.supportsLinkCreationQuery = false;
        m_capabilities.supportsLinkDeletionQuery = false;
        m_capabilities.supportsSelectionQuery = false;
        m_capabilities.backendName = "Fallback";
        m_capabilities.description = "No-op fallback backend (placeholder without active visual editor)";
    }

    NodeEditorBackendType FallbackNodeEditorBackend::backendType() const noexcept
    {
        return NodeEditorBackendType::Fallback;
    }

    const NodeEditorCapabilities& FallbackNodeEditorBackend::capabilities() const noexcept
    {
        return m_capabilities;
    }

    void FallbackNodeEditorBackend::initialize() {}
    void FallbackNodeEditorBackend::shutdown() {}

    void FallbackNodeEditorBackend::beginCanvas(const char* /*editorId*/) {}
    void FallbackNodeEditorBackend::endCanvas() {}

    void FallbackNodeEditorBackend::beginNode(NodeId /*nodeId*/) {}
    void FallbackNodeEditorBackend::endNode() {}

    void FallbackNodeEditorBackend::beginNodeTitle() {}
    void FallbackNodeEditorBackend::endNodeTitle() {}

    void FallbackNodeEditorBackend::beginInputPin(PinId /*pinId*/) {}
    void FallbackNodeEditorBackend::endInputPin() {}

    void FallbackNodeEditorBackend::beginOutputPin(PinId /*pinId*/) {}
    void FallbackNodeEditorBackend::endOutputPin() {}

    void FallbackNodeEditorBackend::drawLink(LinkId /*linkId*/, PinId /*startPinId*/, PinId /*endPinId*/) {}

    std::optional<CreatedLinkEvent> FallbackNodeEditorBackend::queryCreatedLink()
    {
        return std::nullopt;
    }

    std::optional<DeletedLinkEvent> FallbackNodeEditorBackend::queryDeletedLink()
    {
        return std::nullopt;
    }

    std::vector<NodeId> FallbackNodeEditorBackend::querySelectedNodes()
    {
        return {};
    }

    std::vector<LinkId> FallbackNodeEditorBackend::querySelectedLinks()
    {
        return {};
    }

    void FallbackNodeEditorBackend::setNodePosition(NodeId /*nodeId*/, const NodePosition& /*position*/) {}

    std::optional<NodePosition> FallbackNodeEditorBackend::getNodePosition(NodeId /*nodeId*/) const
    {
        return std::nullopt;
    }

    namespace
    {
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
        std::optional<NodeEditorBackendType> s_userPreferredBackend{ std::nullopt };
    } // namespace

    bool isBackendAvailable(NodeEditorBackendType type) noexcept
    {
        switch (type)
        {
        case NodeEditorBackendType::Fallback: [[fallthrough]];
        case NodeEditorBackendType::CustomCanvas: return true;
#ifdef AUTOINPUT_HAS_IMNODES
        case NodeEditorBackendType::ImNodes: return true;
#endif
#ifdef AUTOINPUT_HAS_IMGUI_NODE_EDITOR
        case NodeEditorBackendType::ImguiNodeEditor: return true;
#endif
        default: return false;
        }
    }

    NodeEditorBackendType getPreferredBackendType() noexcept
    {
        if (s_userPreferredBackend.has_value() && isBackendAvailable(*s_userPreferredBackend))
        {
            return *s_userPreferredBackend;
        }

#ifdef AUTOINPUT_HAS_IMGUI_NODE_EDITOR
        return NodeEditorBackendType::ImguiNodeEditor;
#elifdef AUTOINPUT_HAS_IMNODES
        return NodeEditorBackendType::ImNodes;
#else
        return NodeEditorBackendType::Fallback;
#endif
    }

    void setPreferredBackendType(std::optional<NodeEditorBackendType> type) noexcept
    {
        if (type.has_value() && !isBackendAvailable(*type))
        {
            s_userPreferredBackend = std::nullopt;
            return;
        }
        s_userPreferredBackend = type;
    }

    void resetPreferredBackendType() noexcept
    {
        s_userPreferredBackend = std::nullopt;
    }

    std::unique_ptr<INodeEditorBackend> createNodeEditorBackend(NodeEditorBackendType type)
    {
        switch (type)
        {
        case NodeEditorBackendType::Fallback: return std::make_unique<FallbackNodeEditorBackend>();
        case NodeEditorBackendType::CustomCanvas: return createCustomCanvasBackend();
#ifdef AUTOINPUT_HAS_IMNODES
        case NodeEditorBackendType::ImNodes: return createImnodesBackend();
#endif
#ifdef AUTOINPUT_HAS_IMGUI_NODE_EDITOR
        case NodeEditorBackendType::ImguiNodeEditor: return createImguiNodeEditorBackend();
#endif
        default: return nullptr;
        }
    }

    std::unique_ptr<INodeEditorBackend> createDefaultNodeEditorBackend()
    {
        return createNodeEditorBackend(getPreferredBackendType());
    }

} // namespace autoinput::ui::graph
