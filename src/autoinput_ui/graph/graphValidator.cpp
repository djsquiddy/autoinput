/**
 * @file graphValidator.cpp
 * @brief Implementation of graph validation utilities for AutoInput UI visual editors.
 * @author djsquiddy
 * @date August 2026
 */
#include "graphValidator.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace autoinput::ui::graph
{
    namespace
    {
        enum class TraversalColor : std::uint8_t
        {
            White,
            Gray,
            Black
        };

        [[nodiscard]] std::string resolveNodeDisplayName(const GraphNode* node, NodeId fallbackId)
        {
            if (node != nullptr)
            {
                if (!node->title.empty())
                {
                    return node->title;
                }
                return std::string(nodeKindToString(node->kind));
            }
            return std::to_string(fallbackId);
        }

        void detectCyclesDfs(NodeId currentId, const std::unordered_map<NodeId, std::vector<NodeId>>& adjacency,
                             std::unordered_map<NodeId, TraversalColor>& colors,
                             std::unordered_set<NodeId>& reportedCycleNodes, const GraphDocument& doc,
                             std::vector<ValidationIssue>& issues)
        {
            colors[currentId] = TraversalColor::Gray;

            const auto adjIt = adjacency.find(currentId);
            if (adjIt != adjacency.end())
            {
                for (const NodeId neighborId : adjIt->second)
                {
                    const TraversalColor neighborColor = colors[neighborId];
                    if (neighborColor == TraversalColor::Gray)
                    {
                        if (!reportedCycleNodes.contains(neighborId))
                        {
                            reportedCycleNodes.insert(neighborId);
                            const auto* targetNode = doc.findNode(neighborId);
                            const std::string nodeName = resolveNodeDisplayName(targetNode, neighborId);

                            issues.push_back(
                                ValidationIssue{ .severity = ValidationSeverity::Error,
                                                 .message = "Cycle detected in graph flow involving node '" + nodeName +
                                                            "' (ID: " + std::to_string(neighborId) + ").",
                                                 .nodeId = neighborId,
                                                 .linkId = std::nullopt });
                        }
                    }
                    else if (neighborColor == TraversalColor::White)
                    {
                        detectCyclesDfs(neighborId, adjacency, colors, reportedCycleNodes, doc, issues);
                    }
                }
            }

            colors[currentId] = TraversalColor::Black;
        }
    } // namespace

    std::vector<ValidationIssue> validateStartNodes(const GraphDocument& doc, bool requireStart, bool allowMultiple)
    {
        std::vector<ValidationIssue> issues;
        std::size_t startCount = 0;

        for (const auto& node : doc.nodes())
        {
            if (node.kind == NodeKind::Start)
            {
                ++startCount;
                if (!allowMultiple && startCount > 1)
                {
                    issues.push_back(
                        ValidationIssue{ .severity = ValidationSeverity::Error,
                                         .message = "Multiple Start nodes are not permitted in this graph.",
                                         .nodeId = node.id,
                                         .linkId = std::nullopt });
                }
            }
        }

        if (requireStart && startCount == 0)
        {
            issues.push_back(ValidationIssue{ .severity = ValidationSeverity::Error,
                                              .message = "Graph is missing a required Start node.",
                                              .nodeId = std::nullopt,
                                              .linkId = std::nullopt });
        }

        return issues;
    }

    std::vector<ValidationIssue> validateEndNodes(const GraphDocument& doc, bool requireEnd)
    {
        std::vector<ValidationIssue> issues;
        const auto endCount = static_cast<std::size_t>(std::ranges::count_if(
            doc.nodes(), [](const GraphNode& node) noexcept { return node.kind == NodeKind::End; }));

        if (requireEnd && endCount == 0)
        {
            issues.push_back(ValidationIssue{ .severity = ValidationSeverity::Error,
                                              .message = "Graph is missing a required End node.",
                                              .nodeId = std::nullopt,
                                              .linkId = std::nullopt });
        }

        return issues;
    }

    std::vector<ValidationIssue> validateDisconnectedNodes(const GraphDocument& doc, ValidationSeverity severity)
    {
        std::vector<ValidationIssue> issues;

        // Set of pin IDs that are connected by at least one link
        std::unordered_set<PinId> connectedPins;
        connectedPins.reserve(doc.linkCount() * 2);
        for (const auto& link : doc.links())
        {
            connectedPins.insert(link.fromPinId);
            connectedPins.insert(link.toPinId);
        }

        for (const auto& node : doc.nodes())
        {
            if (node.kind == NodeKind::Comment)
            {
                continue;
            }

            bool hasConnectedLink = false;
            for (const PinId pinId : node.pinIds)
            {
                if (connectedPins.contains(pinId))
                {
                    hasConnectedLink = true;
                    break;
                }
            }

            if (!hasConnectedLink)
            {
                const std::string nodeName =
                    !node.title.empty() ? node.title : std::string(nodeKindToString(node.kind));

                issues.push_back(ValidationIssue{ .severity = severity,
                                                  .message = "Node '" + nodeName + "' (ID: " + std::to_string(node.id) +
                                                             ") is disconnected from the graph topology.",
                                                  .nodeId = node.id,
                                                  .linkId = std::nullopt });
            }
        }

        return issues;
    }

    std::vector<ValidationIssue> validateLinkReferences(const GraphDocument& doc)
    {
        std::vector<ValidationIssue> issues;

        for (const auto& link : doc.links())
        {
            const auto* fromPin = doc.findPin(link.fromPinId);
            const auto* toPin = doc.findPin(link.toPinId);

            if (fromPin == nullptr)
            {
                issues.push_back(ValidationIssue{ .severity = ValidationSeverity::Error,
                                                  .message = "Link (ID: " + std::to_string(link.id) +
                                                             ") references non-existent source pin ID " +
                                                             std::to_string(link.fromPinId) + ".",
                                                  .nodeId = std::nullopt,
                                                  .linkId = link.id });
            }

            if (toPin == nullptr)
            {
                issues.push_back(ValidationIssue{ .severity = ValidationSeverity::Error,
                                                  .message = "Link (ID: " + std::to_string(link.id) +
                                                             ") references non-existent destination pin ID " +
                                                             std::to_string(link.toPinId) + ".",
                                                  .nodeId = std::nullopt,
                                                  .linkId = link.id });
            }
        }

        return issues;
    }

    std::vector<ValidationIssue> validateLinkDirections(const GraphDocument& doc, bool allowSelfLinks)
    {
        std::vector<ValidationIssue> issues;

        for (const auto& link : doc.links())
        {
            const auto* fromPin = doc.findPin(link.fromPinId);
            const auto* toPin = doc.findPin(link.toPinId);

            if (fromPin == nullptr || toPin == nullptr)
            {
                continue;
            }

            if (fromPin->direction == PinDirection::Output && toPin->direction == PinDirection::Output)
            {
                issues.push_back(ValidationIssue{ .severity = ValidationSeverity::Error,
                                                  .message = "Invalid link (ID: " + std::to_string(link.id) +
                                                             "): both source and destination pins are Output pins.",
                                                  .nodeId = fromPin->nodeId,
                                                  .linkId = link.id });
            }
            else if (fromPin->direction == PinDirection::Input && toPin->direction == PinDirection::Input)
            {
                issues.push_back(ValidationIssue{ .severity = ValidationSeverity::Error,
                                                  .message = "Invalid link (ID: " + std::to_string(link.id) +
                                                             "): both source and destination pins are Input pins.",
                                                  .nodeId = fromPin->nodeId,
                                                  .linkId = link.id });
            }
            else if (fromPin->direction == PinDirection::Input && toPin->direction == PinDirection::Output)
            {
                issues.push_back(
                    ValidationIssue{ .severity = ValidationSeverity::Error,
                                     .message = "Invalid link direction (ID: " + std::to_string(link.id) +
                                                "): link originates from an Input pin and connects to an Output pin.",
                                     .nodeId = fromPin->nodeId,
                                     .linkId = link.id });
            }

            if (!allowSelfLinks && fromPin->nodeId == toPin->nodeId)
            {
                issues.push_back(ValidationIssue{ .severity = ValidationSeverity::Error,
                                                  .message = "Invalid self-link (ID: " + std::to_string(link.id) +
                                                             "): link connects pins on the same node.",
                                                  .nodeId = fromPin->nodeId,
                                                  .linkId = link.id });
            }
        }

        return issues;
    }

    std::vector<ValidationIssue> validateAcyclic(const GraphDocument& doc)
    {
        std::vector<ValidationIssue> issues;

        std::unordered_map<NodeId, std::vector<NodeId>> adjacency;
        std::unordered_map<NodeId, TraversalColor> colors;
        std::unordered_set<NodeId> reportedCycleNodes;

        for (const auto& node : doc.nodes())
        {
            colors[node.id] = TraversalColor::White;
        }

        for (const auto& link : doc.links())
        {
            const auto* fromPin = doc.findPin(link.fromPinId);
            const auto* toPin = doc.findPin(link.toPinId);

            if (fromPin == nullptr || toPin == nullptr)
            {
                continue;
            }

            const NodeId srcNode = (fromPin->direction == PinDirection::Output) ? fromPin->nodeId : toPin->nodeId;
            const NodeId dstNode = (fromPin->direction == PinDirection::Output) ? toPin->nodeId : fromPin->nodeId;

            if (srcNode == dstNode)
            {
                if (!reportedCycleNodes.contains(srcNode))
                {
                    reportedCycleNodes.insert(srcNode);
                    const auto* targetNode = doc.findNode(srcNode);
                    const std::string nodeName = resolveNodeDisplayName(targetNode, srcNode);

                    issues.push_back(ValidationIssue{ .severity = ValidationSeverity::Error,
                                                      .message = "Cycle detected: node '" + nodeName +
                                                                 "' (ID: " + std::to_string(srcNode) +
                                                                 ") links directly to itself.",
                                                      .nodeId = srcNode,
                                                      .linkId = link.id });
                }
            }
            else
            {
                adjacency[srcNode].push_back(dstNode);
            }
        }

        for (const auto& node : doc.nodes())
        {
            if (colors[node.id] == TraversalColor::White)
            {
                detectCyclesDfs(node.id, adjacency, colors, reportedCycleNodes, doc, issues);
            }
        }

        return issues;
    }

    ValidationResult validateGraph(const GraphDocument& doc, const ValidationOptions& options)
    {
        ValidationResult result;

        auto appendIssues = [&result](std::vector<ValidationIssue> issuesToAppend)
        {
            for (auto& issue : issuesToAppend)
            {
                result.issues.push_back(std::move(issue));
            }
        };

        appendIssues(validateLinkReferences(doc));
        appendIssues(validateLinkDirections(doc, options.allowSelfLinks));
        appendIssues(validateStartNodes(doc, options.requireStartNode, options.allowMultipleStartNodes));
        appendIssues(validateEndNodes(doc, options.requireEndNode));

        if (!options.allowDisconnectedNodes)
        {
            const ValidationSeverity disconnectedSeverity =
                options.treatDisconnectedAsError ? ValidationSeverity::Error : ValidationSeverity::Warning;
            appendIssues(validateDisconnectedNodes(doc, disconnectedSeverity));
        }

        if (options.requireAcyclic)
        {
            appendIssues(validateAcyclic(doc));
        }

        return result;
    }
} // namespace autoinput::ui::graph
