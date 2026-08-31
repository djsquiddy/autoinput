/**
 * @file nodeEditorBackend.h
 * @brief Node editor backend abstraction, capability reporting, and fallback implementation.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_GRAPH_NODE_EDITOR_BACKEND_H
#define INCLUDE_AUTOINPUT_UI_GRAPH_NODE_EDITOR_BACKEND_H

#include "graphModel.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace autoinput::ui::graph
{
    /**
     * @brief Node editor backend types supported by AutoInput UI.
     */
    enum class NodeEditorBackendType : std::uint8_t
    {
        Fallback,
        ImNodes,
        ImguiNodeEditor
    };

    /**
     * @brief Converts backend type enum to human-readable string view.
     */
    [[nodiscard]] constexpr std::string_view backendTypeToString(NodeEditorBackendType type) noexcept
    {
        switch (type)
        {
        case NodeEditorBackendType::Fallback: return "Fallback";
        case NodeEditorBackendType::ImNodes: return "ImNodes";
        case NodeEditorBackendType::ImguiNodeEditor: return "ImguiNodeEditor";
        default: return "Unknown";
        }
    }

    /**
     * @brief Capabilities reported by a node editor backend.
     */
    struct NodeEditorCapabilities
    {
        bool isAvailable{ false };
        bool supportsCanvas{ false };
        bool supportsPositions{ false };
        bool supportsLinkCreationQuery{ false };
        bool supportsLinkDeletionQuery{ false };
        bool supportsSelectionQuery{ false };
        bool supportsGroups{ false };
        bool supportsComments{ false };
        bool supportsMinimap{ false };
        bool supportsZoom{ false };
        bool supportsMultiSelect{ false };
        std::string backendName{ "Fallback" };
        std::string description{ "No-op fallback backend (placeholder without active visual editor)" };
    };

    /**
     * @brief Event information for a user-created link on the canvas.
     */
    struct CreatedLinkEvent
    {
        PinId fromPinId{ InvalidPinId };
        PinId toPinId{ InvalidPinId };
    };

    /**
     * @brief Event information for a user-deleted link on the canvas.
     */
    struct DeletedLinkEvent
    {
        LinkId linkId{ InvalidLinkId };
    };

    /**
     * @brief Interface for visual graph rendering backends.
     */
    class INodeEditorBackend
    {
    public:
        INodeEditorBackend() = default;
        virtual ~INodeEditorBackend() = default;
        INodeEditorBackend(const INodeEditorBackend&) = delete;
        INodeEditorBackend& operator=(const INodeEditorBackend&) = delete;
        INodeEditorBackend(INodeEditorBackend&&) = delete;
        INodeEditorBackend& operator=(INodeEditorBackend&&) = delete;

        [[nodiscard]] virtual NodeEditorBackendType backendType() const noexcept = 0;
        [[nodiscard]] virtual const NodeEditorCapabilities& capabilities() const noexcept = 0;

        virtual void initialize() = 0;
        virtual void shutdown() = 0;

        virtual void beginCanvas(const char* editorId = "NodeEditor") = 0;
        virtual void endCanvas() = 0;

        virtual void beginNode(NodeId nodeId) = 0;
        virtual void endNode() = 0;

        virtual void beginNodeTitle() = 0;
        virtual void endNodeTitle() = 0;

        virtual void beginInputPin(PinId pinId) = 0;
        virtual void endInputPin() = 0;

        virtual void beginOutputPin(PinId pinId) = 0;
        virtual void endOutputPin() = 0;

        virtual void drawLink(LinkId linkId, PinId startPinId, PinId endPinId) = 0;

        [[nodiscard]] virtual std::optional<CreatedLinkEvent> queryCreatedLink() = 0;
        [[nodiscard]] virtual std::optional<DeletedLinkEvent> queryDeletedLink() = 0;
        [[nodiscard]] virtual std::vector<NodeId> querySelectedNodes() = 0;
        [[nodiscard]] virtual std::vector<LinkId> querySelectedLinks() = 0;

        virtual void setNodePosition(NodeId nodeId, const NodePosition& position) = 0;
        [[nodiscard]] virtual std::optional<NodePosition> getNodePosition(NodeId nodeId) const = 0;
    };

    /**
     * @brief Checks if a specific backend type is available in the current build.
     */
    [[nodiscard]] bool isBackendAvailable(NodeEditorBackendType type) noexcept;

    /**
     * @brief Returns the preferred node editor backend type for the current build.
     */
    [[nodiscard]] NodeEditorBackendType getPreferredBackendType() noexcept;

    /**
     * @brief Sets or clears the user-selected preferred node editor backend type.
     * @param type The preferred backend type, or std::nullopt to reset to automatic build priority.
     */
    void setPreferredBackendType(std::optional<NodeEditorBackendType> type) noexcept;

    /**
     * @brief Resets the preferred backend type to automatic build priority.
     */
    void resetPreferredBackendType() noexcept;

    /**
     * @brief Creates a node editor backend instance of the requested type.
     * @return Unique pointer to the backend, or nullptr if the requested backend is unavailable.
     */
    [[nodiscard]] std::unique_ptr<INodeEditorBackend> createNodeEditorBackend(NodeEditorBackendType type);

    /**
     * @brief Creates a node editor backend instance using the default/preferred backend.
     */
    [[nodiscard]] std::unique_ptr<INodeEditorBackend> createDefaultNodeEditorBackend();

    /**
     * @brief Default no-op fallback backend implementation.
     */
    class FallbackNodeEditorBackend : public INodeEditorBackend
    {
    public:
        FallbackNodeEditorBackend();
        ~FallbackNodeEditorBackend() override = default;
        FallbackNodeEditorBackend(const FallbackNodeEditorBackend&) = delete;
        FallbackNodeEditorBackend& operator=(const FallbackNodeEditorBackend&) = delete;
        FallbackNodeEditorBackend(FallbackNodeEditorBackend&&) = delete;
        FallbackNodeEditorBackend& operator=(FallbackNodeEditorBackend&&) = delete;

        [[nodiscard]] NodeEditorBackendType backendType() const noexcept override;
        [[nodiscard]] const NodeEditorCapabilities& capabilities() const noexcept override;

        void initialize() override;
        void shutdown() override;

        void beginCanvas(const char* editorId = "NodeEditor") override;
        void endCanvas() override;

        void beginNode(NodeId nodeId) override;
        void endNode() override;

        void beginNodeTitle() override;
        void endNodeTitle() override;

        void beginInputPin(PinId pinId) override;
        void endInputPin() override;

        void beginOutputPin(PinId pinId) override;
        void endOutputPin() override;

        void drawLink(LinkId linkId, PinId startPinId, PinId endPinId) override;

        [[nodiscard]] std::optional<CreatedLinkEvent> queryCreatedLink() override;
        [[nodiscard]] std::optional<DeletedLinkEvent> queryDeletedLink() override;
        [[nodiscard]] std::vector<NodeId> querySelectedNodes() override;
        [[nodiscard]] std::vector<LinkId> querySelectedLinks() override;

        void setNodePosition(NodeId nodeId, const NodePosition& position) override;
        [[nodiscard]] std::optional<NodePosition> getNodePosition(NodeId nodeId) const override;

    private:
        NodeEditorCapabilities m_capabilities;
    };

} // namespace autoinput::ui::graph

#endif // INCLUDE_AUTOINPUT_UI_GRAPH_NODE_EDITOR_BACKEND_H
