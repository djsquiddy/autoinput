/**
 * @file graphModel.h
 * @brief Dependency-free graph document model for AutoInput visual editors.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_GRAPH_GRAPH_MODEL_H
#define INCLUDE_AUTOINPUT_UI_GRAPH_GRAPH_MODEL_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace autoinput::ui::graph
{
    /**
     * @brief Unique identifier types for graph elements.
     */
    using NodeId = std::uint64_t;
    using PinId = std::uint64_t;
    using LinkId = std::uint64_t;

    inline constexpr NodeId InvalidNodeId{ 0 };
    inline constexpr PinId InvalidPinId{ 0 };
    inline constexpr LinkId InvalidLinkId{ 0 };

    /**
     * @brief Node kinds supported by the graph model.
     */
    enum class NodeKind : std::uint8_t
    {
        Start,
        End,
        RecordedEvent,
        Wait,
        Command,
        Control,
        Input,
        Sequence,
        ExclusiveGroup,
        ApplicationFilter,
        BlacklistEntry,
        Comment,
        Unknown
    };

    /**
     * @brief Converts a NodeKind enum value to a human-readable string representation.
     */
    [[nodiscard]] constexpr std::string_view nodeKindToString(NodeKind kind) noexcept
    {
        switch (kind)
        {
        case NodeKind::Start: return "Start";
        case NodeKind::End: return "End";
        case NodeKind::RecordedEvent: return "Recorded event";
        case NodeKind::Wait: return "Wait";
        case NodeKind::Command: return "Command";
        case NodeKind::Control: return "Control";
        case NodeKind::Input: return "Input";
        case NodeKind::Sequence: return "Sequence";
        case NodeKind::ExclusiveGroup: return "Exclusive group";
        case NodeKind::ApplicationFilter: return "Application filter";
        case NodeKind::BlacklistEntry: return "Blacklist entry";
        case NodeKind::Comment: return "Comment";
        case NodeKind::Unknown:
        default: return "Unknown";
        }
    }

    /**
     * @brief Pin direction: input or output.
     */
    enum class PinDirection : std::uint8_t
    {
        Input,
        Output
    };

    /**
     * @brief Converts a PinDirection enum value to a human-readable string representation.
     */
    [[nodiscard]] constexpr std::string_view pinDirectionToString(PinDirection direction) noexcept
    {
        switch (direction)
        {
        case PinDirection::Input: return "Input";
        case PinDirection::Output: return "Output";
        default: return "Unknown";
        }
    }

    /**
     * @brief Simple 2D coordinates for node positioning in visual canvases.
     */
    struct NodePosition
    {
        float x{ 0.0F };
        float y{ 0.0F };

        constexpr auto operator<=>(const NodePosition&) const = default;
    };

    /**
     * @brief Metadata for graph nodes.
     */
    struct NodeMetadata
    {
        std::string title;
        std::string subtitle;
        std::optional<std::size_t> sourceIndex{ std::nullopt };

        constexpr auto operator<=>(const NodeMetadata&) const = default;
    };

    /**
     * @brief Graph pin data structure.
     */
    struct GraphPin
    {
        PinId id{ InvalidPinId };
        NodeId nodeId{ InvalidNodeId };
        PinDirection direction{ PinDirection::Input };
        std::string name;

        constexpr auto operator<=>(const GraphPin&) const = default;
    };

    /**
     * @brief Graph link connecting two pins.
     */
    struct GraphLink
    {
        LinkId id{ InvalidLinkId };
        PinId fromPinId{ InvalidPinId };
        PinId toPinId{ InvalidPinId };

        constexpr auto operator<=>(const GraphLink&) const = default;
    };

    /**
     * @brief Graph node data structure.
     */
    struct GraphNode
    {
        NodeId id{ InvalidNodeId };
        NodeKind kind{ NodeKind::Unknown };
        NodePosition position{ .x = 0.0F, .y = 0.0F };
        std::string title;
        std::string subtitle;
        std::optional<std::size_t> sourceIndex{ std::nullopt };
        std::vector<PinId> pinIds;

        [[nodiscard]] const std::string& details() const noexcept { return subtitle; }

        void setDetails(std::string_view detailsText) { subtitle = detailsText; }

        [[nodiscard]] NodeMetadata metadata() const
        {
            return NodeMetadata{ .title = title, .subtitle = subtitle, .sourceIndex = sourceIndex };
        }

        void setMetadata(const NodeMetadata& meta)
        {
            title = meta.title;
            subtitle = meta.subtitle;
            sourceIndex = meta.sourceIndex;
        }
    };

    /**
     * @brief Reusable, dependency-free document model representing an editing graph.
     */
    class GraphDocument
    {
    public:
        GraphDocument() = default;
        ~GraphDocument() = default;

        GraphDocument(const GraphDocument&) = default;
        GraphDocument& operator=(const GraphDocument&) = default;
        GraphDocument(GraphDocument&&) noexcept = default;
        GraphDocument& operator=(GraphDocument&&) noexcept = default;

        // --- Node Operations ---

        /**
         * @brief Creates a new node with an auto-generated unique ID.
         */
        GraphNode& createNode(NodeKind kind = NodeKind::Unknown, std::string_view title = {},
                              NodePosition position = { .x = 0.0F, .y = 0.0F });

        /**
         * @brief Creates a new node with an explicit ID.
         * @return Pointer to created node, or nullptr if ID already exists or is invalid.
         */
        GraphNode* createNodeWithId(NodeId id, NodeKind kind = NodeKind::Unknown, std::string_view title = {},
                                    NodePosition position = { .x = 0.0F, .y = 0.0F });

        /**
         * @brief Finds a node by ID.
         */
        [[nodiscard]] GraphNode* findNode(NodeId id) noexcept;
        [[nodiscard]] const GraphNode* findNode(NodeId id) const noexcept;

        /**
         * @brief Removes a node and all its pins and attached links.
         * @return True if the node existed and was removed.
         */
        bool removeNode(NodeId id);

        // --- Pin Operations ---

        /**
         * @brief Creates a new pin attached to the specified node with an auto-generated unique ID.
         * @return Pointer to created pin, or nullptr if node does not exist.
         */
        GraphPin* createPin(NodeId nodeId, PinDirection direction, std::string_view name = {});

        /**
         * @brief Creates a new pin with an explicit ID attached to the specified node.
         * @return Pointer to created pin, or nullptr if node does not exist, ID is invalid or taken.
         */
        GraphPin* createPinWithId(PinId id, NodeId nodeId, PinDirection direction, std::string_view name = {});

        /**
         * @brief Finds a pin by ID.
         */
        [[nodiscard]] GraphPin* findPin(PinId id) noexcept;
        [[nodiscard]] const GraphPin* findPin(PinId id) const noexcept;

        /**
         * @brief Checks whether a pin belongs to a node.
         */
        [[nodiscard]] bool isPinOnNode(PinId pinId, NodeId nodeId) const noexcept;
        [[nodiscard]] bool pinBelongsToNode(PinId pinId, NodeId nodeId) const noexcept;

        /**
         * @brief Removes a pin and all links attached to it.
         * @return True if pin existed and was removed.
         */
        bool removePin(PinId id);

        // --- Link Operations ---

        /**
         * @brief Creates a new link between two pins with an auto-generated unique ID.
         * @return Pointer to created link, or nullptr if pins are invalid/missing.
         */
        GraphLink* createLink(PinId fromPinId, PinId toPinId);

        /**
         * @brief Creates a new link with an explicit ID.
         * @return Pointer to created link, or nullptr if invalid pins or ID conflict.
         */
        GraphLink* createLinkWithId(LinkId id, PinId fromPinId, PinId toPinId);

        /**
         * @brief Finds a link by ID.
         */
        [[nodiscard]] GraphLink* findLink(LinkId id) noexcept;
        [[nodiscard]] const GraphLink* findLink(LinkId id) const noexcept;

        /**
         * @brief Finds a link connecting two specific pins.
         */
        [[nodiscard]] GraphLink* findLinkBetween(PinId fromPinId, PinId toPinId) noexcept;
        [[nodiscard]] const GraphLink* findLinkBetween(PinId fromPinId, PinId toPinId) const noexcept;

        /**
         * @brief Removes a link by ID.
         * @return True if link existed and was removed.
         */
        bool removeLink(LinkId id);

        /**
         * @brief Removes all links attached to a specific pin.
         * @return Number of links removed.
         */
        std::size_t removeLinksConnectedToPin(PinId pinId);

        // --- Container Access and Utility ---

        [[nodiscard]] const std::vector<GraphNode>& nodes() const noexcept { return m_nodes; }
        [[nodiscard]] std::vector<GraphNode>& nodes() noexcept { return m_nodes; }

        [[nodiscard]] const std::vector<GraphPin>& pins() const noexcept { return m_pins; }
        [[nodiscard]] std::vector<GraphPin>& pins() noexcept { return m_pins; }

        [[nodiscard]] const std::vector<GraphLink>& links() const noexcept { return m_links; }
        [[nodiscard]] std::vector<GraphLink>& links() noexcept { return m_links; }

        [[nodiscard]] std::size_t nodeCount() const noexcept { return m_nodes.size(); }
        [[nodiscard]] std::size_t pinCount() const noexcept { return m_pins.size(); }
        [[nodiscard]] std::size_t linkCount() const noexcept { return m_links.size(); }

        [[nodiscard]] bool empty() const noexcept { return m_nodes.empty(); }

        /**
         * @brief Clears all nodes, pins, and links.
         */
        void clear() noexcept;

    private:
        [[nodiscard]] NodeId allocateNextNodeId() noexcept;
        [[nodiscard]] PinId allocateNextPinId() noexcept;
        [[nodiscard]] LinkId allocateNextLinkId() noexcept;

        std::vector<GraphNode> m_nodes;
        std::vector<GraphPin> m_pins;
        std::vector<GraphLink> m_links;

        NodeId m_nextNodeId{ 1 };
        PinId m_nextPinId{ 1 };
        LinkId m_nextLinkId{ 1 };
    };

    using GraphModel = GraphDocument;
} // namespace autoinput::ui::graph

#endif // INCLUDE_AUTOINPUT_UI_GRAPH_GRAPH_MODEL_H
