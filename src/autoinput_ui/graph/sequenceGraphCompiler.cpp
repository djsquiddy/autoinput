/**
 * @file sequenceGraphCompiler.cpp
 * @brief Compiler implementation transforming a sequence GraphDocument back into a RecordedSequence.
 * @author djsquiddy
 * @date August 2026
 */
#include "sequenceGraphCompiler.h"
#include "autoinput/input/waitDelay.h"

#include <algorithm>
#include <format>
#include <string_view>
#include <unordered_set>

namespace autoinput::ui::graph
{
    autoinput::RecordedEvent parseRecordedEventFromNode(const GraphNode& node)
    {
        autoinput::RecordedEvent event;
        event.delay = "0ms";

        // 1. Determine event type from title
        if (node.title == "Key Down")
        {
            event.type = autoinput::RecordedEventType::KeyDown;
        }
        else if (node.title == "Key Up")
        {
            event.type = autoinput::RecordedEventType::KeyUp;
        }
        else if (node.title == "Mouse Down")
        {
            event.type = autoinput::RecordedEventType::MouseDown;
        }
        else if (node.title == "Mouse Up")
        {
            event.type = autoinput::RecordedEventType::MouseUp;
        }
        else if (node.title == "Mouse Move")
        {
            event.type = autoinput::RecordedEventType::MouseMove;
        }
        else
        {
            event.type = autoinput::RecordedEventType::Invalid;
        }

        // 2. Parse details string
        std::string_view details = node.details();

        // Check for delay suffix: " (delay: 100ms)"
        const auto delayPos = details.find(" (delay: ");
        if (delayPos != std::string_view::npos)
        {
            const auto delayEnd = details.find(')', delayPos + 9);
            if (delayEnd != std::string_view::npos)
            {
                event.delay = std::string(details.substr(delayPos + 9, delayEnd - (delayPos + 9)));
            }
            details = details.substr(0, delayPos);
        }
        else if (details.starts_with("Delay: "))
        {
            event.delay = std::string(details.substr(7));
            return event;
        }

        if (details.starts_with("Key: "))
        {
            std::string keyVal = std::string(details.substr(5));
            if (keyVal != "<none>" && !keyVal.empty())
            {
                event.key = std::move(keyVal);
            }
        }
        else if (details.starts_with("Button: "))
        {
            std::string btnVal = std::string(details.substr(8));
            if (btnVal != "<none>" && !btnVal.empty())
            {
                event.button = std::move(btnVal);
            }
        }
        else if (details.starts_with("Position: ("))
        {
            const auto commaPos = details.find(',', 11);
            const auto closePos = details.find(')', 11);
            if (commaPos != std::string_view::npos && closePos != std::string_view::npos && commaPos < closePos)
            {
                std::string xStr = std::string(details.substr(11, commaPos - 11));
                std::string yStr = std::string(details.substr(commaPos + 1, closePos - (commaPos + 1)));

                while (!xStr.empty() && xStr.front() == ' ')
                {
                    xStr.erase(0, 1);
                }
                while (!xStr.empty() && xStr.back() == ' ')
                {
                    xStr.pop_back();
                }
                while (!yStr.empty() && yStr.front() == ' ')
                {
                    yStr.erase(0, 1);
                }
                while (!yStr.empty() && yStr.back() == ' ')
                {
                    yStr.pop_back();
                }

                try
                {
                    event.x = std::stoi(xStr);
                    event.y = std::stoi(yStr);
                }
                catch (...)
                {
                    event.x = 0;
                    event.y = 0;
                }
            }
        }

        return event;
    }

    SequenceCompileResult compileGraphToSequence(const GraphDocument& doc, const SequenceCompileOptions& options)
    {
        SequenceCompileResult result;

        // 1. Initial structural validation
        auto startIssues = validateStartNodes(doc, true, false);
        result.issues.insert(result.issues.end(), startIssues.begin(), startIssues.end());

        auto endIssues = validateEndNodes(doc, true);
        result.issues.insert(result.issues.end(), endIssues.begin(), endIssues.end());

        auto linkRefIssues = validateLinkReferences(doc);
        result.issues.insert(result.issues.end(), linkRefIssues.begin(), linkRefIssues.end());

        auto linkDirIssues = validateLinkDirections(doc, false);
        result.issues.insert(result.issues.end(), linkDirIssues.begin(), linkDirIssues.end());

        auto cycleIssues = validateAcyclic(doc);
        result.issues.insert(result.issues.end(), cycleIssues.begin(), cycleIssues.end());

        if (result.hasErrors())
        {
            result.success = false;
            return result;
        }

        // 2. Find the Start node
        const GraphNode* startNode = nullptr;
        for (const auto& node : doc.nodes())
        {
            if (node.kind == NodeKind::Start)
            {
                startNode = &node;
                break;
            }
        }

        if (startNode == nullptr)
        {
            result.success = false;
            return result;
        }

        // 3. Verify node degrees and detect unsupported branching / merging
        for (const auto& node : doc.nodes())
        {
            std::vector<const GraphLink*> outgoing;
            std::vector<const GraphLink*> incoming;

            for (const auto& link : doc.links())
            {
                if (doc.pinBelongsToNode(link.fromPinId, node.id))
                {
                    outgoing.push_back(&link);
                }
                if (doc.pinBelongsToNode(link.toPinId, node.id))
                {
                    incoming.push_back(&link);
                }
            }

            if (outgoing.size() > 1)
            {
                result.issues.push_back(ValidationIssue{
                    .severity = ValidationSeverity::Error,
                    .message = std::format(
                        "Unsupported branching: node '{}' has {} outgoing links.", node.title, outgoing.size()),
                    .nodeId = node.id,
                    .linkId = outgoing[0]->id });
            }

            if (incoming.size() > 1)
            {
                result.issues.push_back(ValidationIssue{
                    .severity = ValidationSeverity::Error,
                    .message = std::format(
                        "Unsupported branching/merging: node '{}' has {} incoming links.", node.title, incoming.size()),
                    .nodeId = node.id,
                    .linkId = incoming[0]->id });
            }

            if (node.kind == NodeKind::Start && !incoming.empty())
            {
                result.issues.push_back(ValidationIssue{
                    .severity = ValidationSeverity::Error,
                    .message = std::format("Start node '{}' cannot have incoming execution links.", node.title),
                    .nodeId = node.id,
                    .linkId = incoming[0]->id });
            }

            if (node.kind == NodeKind::End && !outgoing.empty())
            {
                result.issues.push_back(ValidationIssue{
                    .severity = ValidationSeverity::Error,
                    .message = std::format("End node '{}' cannot have outgoing execution links.", node.title),
                    .nodeId = node.id,
                    .linkId = outgoing[0]->id });
            }
        }

        if (result.hasErrors())
        {
            result.success = false;
            return result;
        }

        // 4. Follow execution links to build the linear topological order
        std::vector<const GraphNode*> path;
        std::unordered_set<NodeId> visited;

        const GraphNode* current = startNode;
        path.push_back(current);
        visited.insert(current->id);

        bool reachedEnd = false;

        while (current != nullptr)
        {
            if (current->kind == NodeKind::End)
            {
                reachedEnd = true;
                break;
            }

            const GraphLink* nextLink = nullptr;
            for (const auto& link : doc.links())
            {
                if (doc.pinBelongsToNode(link.fromPinId, current->id))
                {
                    nextLink = &link;
                    break;
                }
            }

            if (nextLink == nullptr)
            {
                result.issues.push_back(ValidationIssue{
                    .severity = ValidationSeverity::Error,
                    .message =
                        std::format("No reachable End node: execution path terminated at node '{}'.", current->title),
                    .nodeId = current->id,
                    .linkId = std::nullopt });
                break;
            }

            const auto* toPin = doc.findPin(nextLink->toPinId);
            if (toPin == nullptr)
            {
                result.issues.push_back(ValidationIssue{ .severity = ValidationSeverity::Error,
                                                         .message = "Link references non-existent destination pin.",
                                                         .nodeId = current->id,
                                                         .linkId = nextLink->id });
                break;
            }

            const auto* nextNode = doc.findNode(toPin->nodeId);
            if (nextNode == nullptr)
            {
                result.issues.push_back(ValidationIssue{ .severity = ValidationSeverity::Error,
                                                         .message = "Link connects to a pin on a non-existent node.",
                                                         .nodeId = current->id,
                                                         .linkId = nextLink->id });
                break;
            }

            if (visited.contains(nextNode->id))
            {
                result.issues.push_back(ValidationIssue{
                    .severity = ValidationSeverity::Error,
                    .message = std::format(
                        "Cycle detected: node '{}' was visited multiple times in execution sequence.", nextNode->title),
                    .nodeId = nextNode->id,
                    .linkId = nextLink->id });
                break;
            }

            visited.insert(nextNode->id);
            path.push_back(nextNode);
            current = nextNode;
        }

        if (!reachedEnd && !result.hasErrors())
        {
            result.issues.push_back(
                ValidationIssue{ .severity = ValidationSeverity::Error,
                                 .message = "Graph execution path does not reach an End node.",
                                 .nodeId = current != nullptr ? std::optional<NodeId>(current->id) : std::nullopt,
                                 .linkId = std::nullopt });
        }

        // 5. Detect disconnected execution nodes
        for (const auto& node : doc.nodes())
        {
            if (!visited.contains(node.id))
            {
                if (node.kind != NodeKind::Comment)
                {
                    result.issues.push_back(
                        ValidationIssue{ .severity = ValidationSeverity::Error,
                                         .message = std::format("Disconnected execution node '{}' of kind '{}' is not "
                                                                "connected to sequence execution path.",
                                                                node.title,
                                                                nodeKindToString(node.kind)),
                                         .nodeId = node.id,
                                         .linkId = std::nullopt });
                }
            }
        }

        // 6. Check that all nodes along execution path are supported kinds
        for (const auto* node : path)
        {
            if (node->kind != NodeKind::Start && node->kind != NodeKind::End && node->kind != NodeKind::RecordedEvent &&
                node->kind != NodeKind::Wait)
            {
                result.issues.push_back(
                    ValidationIssue{ .severity = ValidationSeverity::Error,
                                     .message = std::format("Unsupported node kind '{}' in sequence execution path.",
                                                            nodeKindToString(node->kind)),
                                     .nodeId = node->id,
                                     .linkId = std::nullopt });
            }
        }

        if (result.hasErrors())
        {
            result.success = false;
            return result;
        }

        // 7. Assemble the compiled RecordedSequence
        autoinput::RecordedSequence compiled;
        compiled.name = options.defaultName;
        compiled.start = options.defaultStart;
        compiled.repeat = options.defaultRepeat;

        if (options.sourceSequence.has_value())
        {
            compiled.name = options.sourceSequence->name;
            compiled.start = options.sourceSequence->start;
            compiled.repeat = options.sourceSequence->repeat;
        }
        else
        {
            if (startNode != nullptr)
            {
                const std::string& details = startNode->details();
                if (details.starts_with("Trigger: "))
                {
                    compiled.start = details.substr(9);
                }
                else if (!details.empty())
                {
                    compiled.name = details;
                }
            }

            if (!path.empty())
            {
                const auto* endNode = path.back();
                if (endNode != nullptr && endNode->kind == NodeKind::End)
                {
                    if (endNode->details() == "Repeat: Enabled")
                    {
                        compiled.repeat = true;
                    }
                    else if (endNode->details() == "Repeat: Disabled")
                    {
                        compiled.repeat = false;
                    }
                }
            }
        }

        std::optional<std::string> pendingWaitDelay{ std::nullopt };

        for (std::size_t i = 1; i + 1 < path.size(); ++i)
        {
            const auto* node = path[i];
            if (node->kind == NodeKind::Wait)
            {
                std::string delay = "0ms";
                if (node->sourceIndex.has_value() && options.sourceSequence.has_value() &&
                    *node->sourceIndex < options.sourceSequence->events.size())
                {
                    delay = options.sourceSequence->events[*node->sourceIndex].delay;
                }
                else
                {
                    const std::string& d = node->details();
                    if (d.starts_with("Delay: "))
                    {
                        delay = d.substr(7);
                    }
                    else if (!d.empty())
                    {
                        delay = d;
                    }
                }
                pendingWaitDelay = delay;
            }
            else if (node->kind == NodeKind::RecordedEvent)
            {
                autoinput::RecordedEvent event;
                if (node->sourceIndex.has_value() && options.sourceSequence.has_value() &&
                    *node->sourceIndex < options.sourceSequence->events.size())
                {
                    event = options.sourceSequence->events[*node->sourceIndex];
                }
                else
                {
                    event = parseRecordedEventFromNode(*node);
                }

                if (pendingWaitDelay.has_value())
                {
                    event.delay = *pendingWaitDelay;
                    pendingWaitDelay = std::nullopt;
                }

                compiled.events.push_back(std::move(event));
            }
        }

        if (pendingWaitDelay.has_value())
        {
            autoinput::RecordedEvent trailingWait;
            trailingWait.type = autoinput::RecordedEventType::Invalid;
            trailingWait.delay = *pendingWaitDelay;
            compiled.events.push_back(std::move(trailingWait));
        }

        result.success = true;
        result.sequence = std::move(compiled);
        return result;
    }

} // namespace autoinput::ui::graph
