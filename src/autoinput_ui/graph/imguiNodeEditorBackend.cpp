/**
 * @file imguiNodeEditorBackend.cpp
 * @brief imgui-node-editor-based node editor backend implementation.
 * @author djsquiddy
 * @date August 2026
 */
#include "imguiNodeEditorBackend.h"

#ifdef AUTOINPUT_HAS_IMGUI_NODE_EDITOR
#include <imgui.h>
#include <imgui_node_editor.h>

namespace ed = ax::NodeEditor;

namespace autoinput::ui::graph
{
    class ImguiNodeEditorBackend final : public INodeEditorBackend
    {
    public:
        ImguiNodeEditorBackend()
        {
            m_capabilities.isAvailable = true;
            m_capabilities.supportsCanvas = true;
            m_capabilities.supportsPositions = true;
            m_capabilities.supportsLinkCreationQuery = true;
            m_capabilities.supportsLinkDeletionQuery = true;
            m_capabilities.supportsSelectionQuery = true;
            m_capabilities.supportsGroups = true;
            m_capabilities.supportsComments = true;
            m_capabilities.supportsMinimap = false;
            m_capabilities.supportsZoom = true;
            m_capabilities.supportsMultiSelect = true;
            m_capabilities.backendName = "imgui-node-editor";
            m_capabilities.description = "imgui-node-editor canvas backend for Dear ImGui";
        }

        ~ImguiNodeEditorBackend() override { shutdown(); }

        ImguiNodeEditorBackend(const ImguiNodeEditorBackend&) = delete;
        ImguiNodeEditorBackend& operator=(const ImguiNodeEditorBackend&) = delete;
        ImguiNodeEditorBackend(ImguiNodeEditorBackend&&) = delete;
        ImguiNodeEditorBackend& operator=(ImguiNodeEditorBackend&&) = delete;

        [[nodiscard]] NodeEditorBackendType backendType() const noexcept override
        {
            return NodeEditorBackendType::ImguiNodeEditor;
        }

        [[nodiscard]] const NodeEditorCapabilities& capabilities() const noexcept override { return m_capabilities; }

        void initialize() override
        {
            if (!m_initialized)
            {
                ed::Config config;
                config.SettingsFile = nullptr;
                m_context = ed::CreateEditor(&config);
                m_initialized = true;
            }
        }

        void shutdown() override
        {
            if (m_initialized)
            {
                if (m_context != nullptr)
                {
                    ed::DestroyEditor(m_context);
                    m_context = nullptr;
                }
                m_initialized = false;
            }
        }

        void beginCanvas(const char* editorId = "NodeEditor") override
        {
            if (!m_initialized)
            {
                initialize();
            }
            if (m_context != nullptr)
            {
                ed::SetCurrentEditor(m_context);
            }
            ed::Begin(editorId);
        }

        void endCanvas() override { ed::End(); }

        void beginNode(NodeId nodeId) override { ed::BeginNode(static_cast<ed::NodeId>(nodeId)); }

        void endNode() override { ed::EndNode(); }

        void beginNodeTitle() override {}
        void endNodeTitle() override {}

        void beginInputPin(PinId pinId) override { ed::BeginPin(static_cast<ed::PinId>(pinId), ed::PinKind::Input); }
        void endInputPin() override { ed::EndPin(); }

        void beginOutputPin(PinId pinId) override { ed::BeginPin(static_cast<ed::PinId>(pinId), ed::PinKind::Output); }
        void endOutputPin() override { ed::EndPin(); }

        void drawLink(LinkId linkId, PinId startPinId, PinId endPinId) override
        {
            ed::Link(static_cast<ed::LinkId>(linkId), static_cast<ed::PinId>(startPinId),
                     static_cast<ed::PinId>(endPinId));
        }

        [[nodiscard]] std::optional<CreatedLinkEvent> queryCreatedLink() override
        {
            if (ed::BeginCreate())
            {
                ed::PinId startPinId;
                ed::PinId endPinId;
                if (ed::QueryNewLink(&startPinId, &endPinId))
                {
                    if (startPinId && endPinId)
                    {
                        if (ed::AcceptNewItem())
                        {
                            ed::EndCreate();
                            return CreatedLinkEvent{ .fromPinId = static_cast<PinId>(startPinId.Get()),
                                                     .toPinId = static_cast<PinId>(endPinId.Get()) };
                        }
                    }
                }
                ed::EndCreate();
            }
            return std::nullopt;
        }

        [[nodiscard]] std::optional<DeletedLinkEvent> queryDeletedLink() override
        {
            if (ed::BeginDelete())
            {
                ed::LinkId deletedLinkId;
                if (ed::QueryDeletedLink(&deletedLinkId))
                {
                    if (ed::AcceptDeletedItem())
                    {
                        ed::EndDelete();
                        return DeletedLinkEvent{ .linkId = static_cast<LinkId>(deletedLinkId.Get()) };
                    }
                }
                ed::EndDelete();
            }
            return std::nullopt;
        }

        [[nodiscard]] std::vector<NodeId> querySelectedNodes() override
        {
            const int count = ed::GetSelectedObjectCount();
            if (count <= 0)
            {
                return {};
            }
            std::vector<ed::NodeId> selectedNodes(count);
            const int actualCount = ed::GetSelectedNodes(selectedNodes.data(), count);
            std::vector<NodeId> result;
            result.reserve(actualCount);
            for (int i = 0; i < actualCount; ++i)
            {
                result.push_back(static_cast<NodeId>(selectedNodes[i].Get()));
            }
            return result;
        }

        [[nodiscard]] std::vector<LinkId> querySelectedLinks() override
        {
            const int count = ed::GetSelectedObjectCount();
            if (count <= 0)
            {
                return {};
            }
            std::vector<ed::LinkId> selectedLinks(count);
            const int actualCount = ed::GetSelectedLinks(selectedLinks.data(), count);
            std::vector<LinkId> result;
            result.reserve(actualCount);
            for (int i = 0; i < actualCount; ++i)
            {
                result.push_back(static_cast<LinkId>(selectedLinks[i].Get()));
            }
            return result;
        }

        void setNodePosition(NodeId nodeId, const NodePosition& position) override
        {
            ed::SetNodePosition(static_cast<ed::NodeId>(nodeId), ImVec2(position.x, position.y));
        }

        [[nodiscard]] std::optional<NodePosition> getNodePosition(NodeId nodeId) const override
        {
            const ImVec2 pos = ed::GetNodePosition(static_cast<ed::NodeId>(nodeId));
            return NodePosition{ .x = pos.x, .y = pos.y };
        }

    private:
        NodeEditorCapabilities m_capabilities;
        ed::EditorContext* m_context{ nullptr };
        bool m_initialized{ false };
    };

    std::unique_ptr<INodeEditorBackend> createImguiNodeEditorBackend()
    {
        return std::make_unique<ImguiNodeEditorBackend>();
    }

} // namespace autoinput::ui::graph
#else
namespace autoinput::ui::graph
{
    std::unique_ptr<INodeEditorBackend> createImguiNodeEditorBackend()
    {
        return nullptr;
    }
} // namespace autoinput::ui::graph
#endif
