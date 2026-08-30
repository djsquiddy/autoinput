/**
 * @file graphModel.cpp
 * @brief Implementation of the dependency-free graph document model.
 * @author djsquiddy
 * @date August 2026
 */
#include "graphModel.h"

#include <algorithm>

namespace autoinput::ui::graph
{
    NodeId GraphDocument::allocateNextNodeId() noexcept
    {
        while (m_nextNodeId == InvalidNodeId || findNode(m_nextNodeId) != nullptr)
        {
            ++m_nextNodeId;
        }
        return m_nextNodeId++;
    }

    PinId GraphDocument::allocateNextPinId() noexcept
    {
        while (m_nextPinId == InvalidPinId || findPin(m_nextPinId) != nullptr)
        {
            ++m_nextPinId;
        }
        return m_nextPinId++;
    }

    LinkId GraphDocument::allocateNextLinkId() noexcept
    {
        while (m_nextLinkId == InvalidLinkId || findLink(m_nextLinkId) != nullptr)
        {
            ++m_nextLinkId;
        }
        return m_nextLinkId++;
    }

    GraphNode& GraphDocument::createNode(NodeKind kind, std::string_view title, NodePosition position)
    {
        const NodeId id = allocateNextNodeId();
        m_nodes.push_back(GraphNode{ .id = id,
                                     .kind = kind,
                                     .position = position,
                                     .title = std::string(title),
                                     .subtitle = {},
                                     .sourceIndex = std::nullopt,
                                     .pinIds = {} });
        return m_nodes.back();
    }

    GraphNode* GraphDocument::createNodeWithId(NodeId id, NodeKind kind, std::string_view title, NodePosition position)
    {
        if (id == InvalidNodeId || findNode(id) != nullptr)
        {
            return nullptr;
        }

        if (id >= m_nextNodeId)
        {
            m_nextNodeId = id + 1;
        }

        m_nodes.push_back(GraphNode{ .id = id,
                                     .kind = kind,
                                     .position = position,
                                     .title = std::string(title),
                                     .subtitle = {},
                                     .sourceIndex = std::nullopt,
                                     .pinIds = {} });
        return &m_nodes.back();
    }

    GraphNode* GraphDocument::findNode(NodeId id) noexcept
    {
        if (id == InvalidNodeId)
        {
            return nullptr;
        }

        const auto it = std::ranges::find_if(m_nodes, [id](const GraphNode& node) noexcept { return node.id == id; });

        return it != m_nodes.end() ? &(*it) : nullptr;
    }

    const GraphNode* GraphDocument::findNode(NodeId id) const noexcept
    {
        if (id == InvalidNodeId)
        {
            return nullptr;
        }

        const auto it = std::ranges::find_if(m_nodes, [id](const GraphNode& node) noexcept { return node.id == id; });

        return it != m_nodes.end() ? &(*it) : nullptr;
    }

    bool GraphDocument::removeNode(NodeId id)
    {
        if (id == InvalidNodeId)
        {
            return false;
        }

        const auto nodeIt =
            std::ranges::find_if(m_nodes, [id](const GraphNode& node) noexcept { return node.id == id; });

        if (nodeIt == m_nodes.end())
        {
            return false;
        }

        // Collect pin IDs belonging to this node
        std::vector<PinId> attachedPins;
        for (const auto& pin : m_pins)
        {
            if (pin.nodeId == id)
            {
                attachedPins.push_back(pin.id);
            }
        }

        // Remove links connected to any of the node's pins
        std::erase_if(m_links,
                      [&attachedPins](const GraphLink& link) noexcept
                      {
                          return std::ranges::find(attachedPins, link.fromPinId) != attachedPins.end() ||
                                 std::ranges::find(attachedPins, link.toPinId) != attachedPins.end();
                      });

        // Remove the pins themselves
        std::erase_if(m_pins, [id](const GraphPin& pin) noexcept { return pin.nodeId == id; });

        // Remove the node
        m_nodes.erase(nodeIt);
        return true;
    }

    GraphPin* GraphDocument::createPin(NodeId nodeId, PinDirection direction, std::string_view name)
    {
        auto* node = findNode(nodeId);
        if (node == nullptr)
        {
            return nullptr;
        }

        const PinId id = allocateNextPinId();
        m_pins.push_back(GraphPin{ .id = id, .nodeId = nodeId, .direction = direction, .name = std::string(name) });

        node->pinIds.push_back(id);
        return &m_pins.back();
    }

    GraphPin* GraphDocument::createPinWithId(PinId id, NodeId nodeId, PinDirection direction, std::string_view name)
    {
        if (id == InvalidPinId || findPin(id) != nullptr)
        {
            return nullptr;
        }

        auto* node = findNode(nodeId);
        if (node == nullptr)
        {
            return nullptr;
        }

        if (id >= m_nextPinId)
        {
            m_nextPinId = id + 1;
        }

        m_pins.push_back(GraphPin{ .id = id, .nodeId = nodeId, .direction = direction, .name = std::string(name) });

        node->pinIds.push_back(id);
        return &m_pins.back();
    }

    GraphPin* GraphDocument::findPin(PinId id) noexcept
    {
        if (id == InvalidPinId)
        {
            return nullptr;
        }

        const auto it = std::ranges::find_if(m_pins, [id](const GraphPin& pin) noexcept { return pin.id == id; });

        return it != m_pins.end() ? &(*it) : nullptr;
    }

    const GraphPin* GraphDocument::findPin(PinId id) const noexcept
    {
        if (id == InvalidPinId)
        {
            return nullptr;
        }

        const auto it = std::ranges::find_if(m_pins, [id](const GraphPin& pin) noexcept { return pin.id == id; });

        return it != m_pins.end() ? &(*it) : nullptr;
    }

    bool GraphDocument::isPinOnNode(PinId pinId, NodeId nodeId) const noexcept
    {
        if (pinId == InvalidPinId || nodeId == InvalidNodeId)
        {
            return false;
        }

        const auto* pin = findPin(pinId);
        return pin != nullptr && pin->nodeId == nodeId;
    }

    bool GraphDocument::pinBelongsToNode(PinId pinId, NodeId nodeId) const noexcept
    {
        return isPinOnNode(pinId, nodeId);
    }

    bool GraphDocument::removePin(PinId id)
    {
        if (id == InvalidPinId)
        {
            return false;
        }

        const auto pinIt = std::ranges::find_if(m_pins, [id](const GraphPin& pin) noexcept { return pin.id == id; });

        if (pinIt == m_pins.end())
        {
            return false;
        }

        const NodeId parentNodeId = pinIt->nodeId;

        // Remove attached links
        removeLinksConnectedToPin(id);

        // Remove from parent node's pin list
        if (auto* node = findNode(parentNodeId))
        {
            std::erase(node->pinIds, id);
        }

        m_pins.erase(pinIt);
        return true;
    }

    GraphLink* GraphDocument::createLink(PinId fromPinId, PinId toPinId)
    {
        if (fromPinId == InvalidPinId || toPinId == InvalidPinId || fromPinId == toPinId)
        {
            return nullptr;
        }

        if (findPin(fromPinId) == nullptr || findPin(toPinId) == nullptr)
        {
            return nullptr;
        }

        const LinkId id = allocateNextLinkId();
        m_links.push_back(GraphLink{ .id = id, .fromPinId = fromPinId, .toPinId = toPinId });

        return &m_links.back();
    }

    GraphLink* GraphDocument::createLinkWithId(LinkId id, PinId fromPinId, PinId toPinId)
    {
        if (id == InvalidLinkId || findLink(id) != nullptr)
        {
            return nullptr;
        }

        if (fromPinId == InvalidPinId || toPinId == InvalidPinId || fromPinId == toPinId)
        {
            return nullptr;
        }

        if (findPin(fromPinId) == nullptr || findPin(toPinId) == nullptr)
        {
            return nullptr;
        }

        if (id >= m_nextLinkId)
        {
            m_nextLinkId = id + 1;
        }

        m_links.push_back(GraphLink{ .id = id, .fromPinId = fromPinId, .toPinId = toPinId });

        return &m_links.back();
    }

    GraphLink* GraphDocument::findLink(LinkId id) noexcept
    {
        if (id == InvalidLinkId)
        {
            return nullptr;
        }

        const auto it = std::ranges::find_if(m_links, [id](const GraphLink& link) noexcept { return link.id == id; });

        return it != m_links.end() ? &(*it) : nullptr;
    }

    const GraphLink* GraphDocument::findLink(LinkId id) const noexcept
    {
        if (id == InvalidLinkId)
        {
            return nullptr;
        }

        const auto it = std::ranges::find_if(m_links, [id](const GraphLink& link) noexcept { return link.id == id; });

        return it != m_links.end() ? &(*it) : nullptr;
    }

    GraphLink* GraphDocument::findLinkBetween(PinId fromPinId, PinId toPinId) noexcept
    {
        if (fromPinId == InvalidPinId || toPinId == InvalidPinId)
        {
            return nullptr;
        }

        const auto it = std::ranges::find_if(m_links, [fromPinId, toPinId](const GraphLink& link) noexcept
                                             { return link.fromPinId == fromPinId && link.toPinId == toPinId; });

        return it != m_links.end() ? &(*it) : nullptr;
    }

    const GraphLink* GraphDocument::findLinkBetween(PinId fromPinId, PinId toPinId) const noexcept
    {
        if (fromPinId == InvalidPinId || toPinId == InvalidPinId)
        {
            return nullptr;
        }

        const auto it = std::ranges::find_if(m_links, [fromPinId, toPinId](const GraphLink& link) noexcept
                                             { return link.fromPinId == fromPinId && link.toPinId == toPinId; });

        return it != m_links.end() ? &(*it) : nullptr;
    }

    bool GraphDocument::removeLink(LinkId id)
    {
        if (id == InvalidLinkId)
        {
            return false;
        }

        const auto it = std::ranges::find_if(m_links, [id](const GraphLink& link) noexcept { return link.id == id; });

        if (it == m_links.end())
        {
            return false;
        }

        m_links.erase(it);
        return true;
    }

    std::size_t GraphDocument::removeLinksConnectedToPin(PinId pinId)
    {
        if (pinId == InvalidPinId)
        {
            return 0;
        }

        return std::erase_if(m_links, [pinId](const GraphLink& link) noexcept
                             { return link.fromPinId == pinId || link.toPinId == pinId; });
    }

    void GraphDocument::clear() noexcept
    {
        m_nodes.clear();
        m_pins.clear();
        m_links.clear();
        m_nextNodeId = 1;
        m_nextPinId = 1;
        m_nextLinkId = 1;
    }
} // namespace autoinput::ui::graph
