/**
 * @file imnodesBackend.cpp
 * @brief imnodes-based node editor backend implementation.
 * @author djsquiddy
 * @date August 2026
 */
#include "imnodesBackend.h"

#ifdef AUTOINPUT_HAS_IMNODES
#include <imgui.h>
#include <imnodes.h>

namespace autoinput::ui::graph
{
    class ImnodesNodeEditorBackend final : public INodeEditorBackend
    {
    public:
        ImnodesNodeEditorBackend()
        {
            m_capabilities.isAvailable = true;
            m_capabilities.supportsCanvas = true;
            m_capabilities.supportsPositions = true;
            m_capabilities.supportsLinkCreationQuery = true;
            m_capabilities.supportsLinkDeletionQuery = true;
            m_capabilities.supportsSelectionQuery = true;
            m_capabilities.backendName = "imnodes";
            m_capabilities.description = "imnodes immediate-mode node editor backend for Dear ImGui";
        }

        ~ImnodesNodeEditorBackend() override { shutdown(); }

        ImnodesNodeEditorBackend(const ImnodesNodeEditorBackend&) = delete;
        ImnodesNodeEditorBackend& operator=(const ImnodesNodeEditorBackend&) = delete;
        ImnodesNodeEditorBackend(ImnodesNodeEditorBackend&&) = delete;
        ImnodesNodeEditorBackend& operator=(ImnodesNodeEditorBackend&&) = delete;

        [[nodiscard]] NodeEditorBackendType backendType() const noexcept override
        {
            return NodeEditorBackendType::ImNodes;
        }

        [[nodiscard]] const NodeEditorCapabilities& capabilities() const noexcept override { return m_capabilities; }

        void initialize() override
        {
            if (!m_initialized)
            {
                m_context = imnodes::EditorContextCreate();
                if (m_context != nullptr)
                {
                    imnodes::EditorContextSet(m_context);
                }
                m_initialized = true;
            }
        }

        void shutdown() override
        {
            if (m_initialized)
            {
                if (m_context != nullptr)
                {
                    imnodes::EditorContextFree(m_context);
                    m_context = nullptr;
                }
                m_initialized = false;
            }
        }

        void beginCanvas(const char* /*editorId*/) override
        {
            if (!m_initialized)
            {
                initialize();
            }
            if (m_context != nullptr)
            {
                imnodes::EditorContextSet(m_context);
            }
            imnodes::BeginNodeEditor();
        }

        void endCanvas() override { imnodes::EndNodeEditor(); }

        void beginNode(NodeId nodeId) override { imnodes::BeginNode(static_cast<int>(nodeId)); }

        void endNode() override { imnodes::EndNode(); }

        void beginNodeTitle() override { imnodes::BeginNodeTitleBar(); }

        void endNodeTitle() override { imnodes::EndNodeTitleBar(); }

        void beginInputPin(PinId pinId) override { imnodes::BeginInputAttribute(static_cast<int>(pinId)); }

        void endInputPin() override { imnodes::EndInputAttribute(); }

        void beginOutputPin(PinId pinId) override { imnodes::BeginOutputAttribute(static_cast<int>(pinId)); }

        void endOutputPin() override { imnodes::EndOutputAttribute(); }

        void drawLink(LinkId linkId, PinId startPinId, PinId endPinId) override
        {
            imnodes::Link(static_cast<int>(linkId), static_cast<int>(startPinId), static_cast<int>(endPinId));
        }

        [[nodiscard]] std::optional<CreatedLinkEvent> queryCreatedLink() override
        {
            int startPin = 0;
            int endPin = 0;
            if (imnodes::IsLinkCreated(&startPin, &endPin))
            {
                return CreatedLinkEvent{ .fromPinId = static_cast<PinId>(startPin),
                                         .toPinId = static_cast<PinId>(endPin) };
            }
            return std::nullopt;
        }

        [[nodiscard]] std::optional<DeletedLinkEvent> queryDeletedLink() override
        {
            int linkId = 0;
            if (imnodes::IsLinkDestroyed(&linkId))
            {
                return DeletedLinkEvent{ .linkId = static_cast<LinkId>(linkId) };
            }
            return std::nullopt;
        }

        [[nodiscard]] std::vector<NodeId> querySelectedNodes() override
        {
            const int count = imnodes::NumSelectedNodes();
            if (count <= 0)
            {
                return {};
            }
            std::vector<int> raw(static_cast<std::size_t>(count));
            imnodes::GetSelectedNodes(raw.data());
            std::vector<NodeId> result;
            result.reserve(raw.size());
            for (const int id : raw)
            {
                result.push_back(static_cast<NodeId>(id));
            }
            return result;
        }

        [[nodiscard]] std::vector<LinkId> querySelectedLinks() override
        {
            const int count = imnodes::NumSelectedLinks();
            if (count <= 0)
            {
                return {};
            }
            std::vector<int> raw(static_cast<std::size_t>(count));
            imnodes::GetSelectedLinks(raw.data());
            std::vector<LinkId> result;
            result.reserve(raw.size());
            for (const int id : raw)
            {
                result.push_back(static_cast<LinkId>(id));
            }
            return result;
        }

        void setNodePosition(NodeId nodeId, const NodePosition& position) override
        {
            imnodes::SetNodeEditorSpacePos(static_cast<int>(nodeId), ImVec2(position.x, position.y));
        }

        [[nodiscard]] std::optional<NodePosition> getNodePosition(NodeId nodeId) const override
        {
            const ImVec2 pos = imnodes::GetNodeEditorSpacePos(static_cast<int>(nodeId));
            return NodePosition{ .x = pos.x, .y = pos.y };
        }

    private:
        NodeEditorCapabilities m_capabilities;
        imnodes::EditorContext* m_context{ nullptr };
        bool m_initialized{ false };
    };

    std::unique_ptr<INodeEditorBackend> createImnodesBackend()
    {
        return std::make_unique<ImnodesNodeEditorBackend>();
    }

} // namespace autoinput::ui::graph

#else

namespace autoinput::ui::graph
{
    std::unique_ptr<INodeEditorBackend> createImnodesBackend()
    {
        return nullptr;
    }

} // namespace autoinput::ui::graph

#endif // AUTOINPUT_HAS_IMNODES
