/**
 * @file sequenceGraphEditor.cpp
 * @brief Implementation of sequence graph editor and inspection UI component.
 * @author djsquiddy
 * @date August 2026
 */
#include "sequenceGraphEditor.h"

#include <algorithm>
#include <cstring>
#include <format>
#include <string_view>
#include <unordered_set>

#if __has_include(<imgui.h>)
#include <imgui.h>
#define AUTOINPUT_UI_INTERNAL_HAS_IMGUI 1
#endif

namespace autoinput::ui::editors
{
    void SequenceGraphEditorState::syncWithSequence(const autoinput::RecordedSequence& sequence, bool force)
    {
        if (!isGraphSynchronized || force || sequence.events.size() != cachedSequenceEventCount ||
            sequence.name != compileOptions.defaultName || sequence.start != compileOptions.defaultStart ||
            sequence.repeat != compileOptions.defaultRepeat)
        {
            rebuildFromSequence(sequence);
        }
    }

    void SequenceGraphEditorState::rebuildFromSequence(const autoinput::RecordedSequence& sequence)
    {
        graphDocument = graph::sequenceToGraphDocument(sequence, adapterOptions);
        compileOptions.sourceSequence = sequence;
        compileOptions.defaultName = sequence.name;
        compileOptions.defaultStart = sequence.start;
        compileOptions.defaultRepeat = sequence.repeat;
        cachedSequenceEventCount = sequence.events.size();

        clearUndoRedo();
        validateCurrentGraph();
        isGraphSynchronized = true;
        statusMessage = std::format("Graph synchronized with sequence ({} nodes).", graphDocument.nodeCount());
    }

    bool SequenceGraphEditorState::validateCurrentGraph()
    {
        auto opts = graph::ValidationOptions::sequenceGraph();
        opts.treatDisconnectedAsError = true;
        validationResult = graph::validateGraph(graphDocument, opts);
        if (validationResult.isValid())
        {
            statusMessage = "Graph validation passed: valid linear sequence topology.";
        }
        else
        {
            statusMessage = std::format("Graph validation found {} issue(s).", validationResult.issues.size());
        }
        return validationResult.isValid();
    }

    graph::SequenceCompileResult SequenceGraphEditorState::compileGraph(
        const std::optional<autoinput::RecordedSequence>& sourceContext)
    {
        graph::SequenceCompileOptions opts = compileOptions;
        if (sourceContext.has_value() && (!sourceContext->name.empty() || !sourceContext->events.empty()))
        {
            opts.sourceSequence = sourceContext;
            if (!sourceContext->name.empty())
            {
                opts.defaultName = sourceContext->name;
            }
            if (!sourceContext->start.empty())
            {
                opts.defaultStart = sourceContext->start;
            }
            opts.defaultRepeat = sourceContext->repeat;
        }

        auto result = graph::compileGraphToSequence(graphDocument, opts);
        if (!result.success)
        {
            lastCompilationError = result.issues.empty() ? "Unknown compilation error" : result.issues.front().message;
            statusMessage = std::format("Compilation failed: {}", *lastCompilationError);
        }
        else
        {
            lastCompilationError = std::nullopt;
            statusMessage = std::format("Graph compiled successfully ({} events).",
                                        result.sequence ? result.sequence->events.size() : 0U);
        }
        return result;
    }

    bool SequenceGraphEditorState::applyToSequence(autoinput::RecordedSequence& targetSequence)
    {
        if (!validateCurrentGraph())
        {
            lastCompilationError =
                validationResult.issues.empty() ? "Graph validation failed." : validationResult.issues.front().message;
            statusMessage = std::format("Cannot apply changes: {}", *lastCompilationError);
            return false;
        }

        auto result = compileGraph(compileOptions.sourceSequence);
        if (result.success && result.sequence.has_value())
        {
            targetSequence = std::move(*result.sequence);
            cachedSequenceEventCount = targetSequence.events.size();
            isGraphSynchronized = true;
            statusMessage = "Graph successfully compiled and applied to sequence.";
            return true;
        }

        lastCompilationError = result.issues.empty() ? "Compilation failed." : result.issues.front().message;
        statusMessage = std::format("Cannot apply changes: {}", *lastCompilationError);
        return false;
    }

    std::optional<SelectedNodeInspectionDetails> SequenceGraphEditorState::getSelectedNodeDetails(
        const autoinput::RecordedSequence& sequence) const
    {
        return resolveNodeInspectionDetails(graphDocument, viewerState.selectedNodeId, sequence, validationResult);
    }

    void SequenceGraphEditorState::selectNode(graph::NodeId nodeId)
    {
        viewerState.selectNode(nodeId);
    }

    void SequenceGraphEditorState::clearSelection()
    {
        viewerState.clearSelection();
    }

    graph::NodeId SequenceGraphEditorState::getSelectedNodeId() const noexcept
    {
        return viewerState.selectedNodeId;
    }

    bool SequenceGraphEditorState::hasSelection() const noexcept
    {
        return viewerState.hasSelection();
    }

    void SequenceGraphEditorState::pushUndoSnapshot()
    {
        undoStack.push_back(GraphEditorSnapshot{ .graphDocument = graphDocument,
                                                 .selectedNodeId = viewerState.selectedNodeId,
                                                 .statusMessage = statusMessage });
        if (undoStack.size() > maxUndoSteps)
        {
            undoStack.erase(undoStack.begin());
        }
        redoStack.clear();
    }

    bool SequenceGraphEditorState::canUndo() const noexcept
    {
        return !undoStack.empty();
    }

    bool SequenceGraphEditorState::canRedo() const noexcept
    {
        return !redoStack.empty();
    }

    bool SequenceGraphEditorState::undo()
    {
        if (!canUndo())
        {
            return false;
        }
        redoStack.push_back(GraphEditorSnapshot{ .graphDocument = graphDocument,
                                                 .selectedNodeId = viewerState.selectedNodeId,
                                                 .statusMessage = statusMessage });

        auto snapshot = std::move(undoStack.back());
        undoStack.pop_back();

        graphDocument = std::move(snapshot.graphDocument);
        viewerState.selectedNodeId = snapshot.selectedNodeId;
        statusMessage = "Undo performed.";
        validateCurrentGraph();
        return true;
    }

    bool SequenceGraphEditorState::redo()
    {
        if (!canRedo())
        {
            return false;
        }
        undoStack.push_back(GraphEditorSnapshot{ .graphDocument = graphDocument,
                                                 .selectedNodeId = viewerState.selectedNodeId,
                                                 .statusMessage = statusMessage });

        auto snapshot = std::move(redoStack.back());
        redoStack.pop_back();

        graphDocument = std::move(snapshot.graphDocument);
        viewerState.selectedNodeId = snapshot.selectedNodeId;
        statusMessage = "Redo performed.";
        validateCurrentGraph();
        return true;
    }

    void SequenceGraphEditorState::clearUndoRedo()
    {
        undoStack.clear();
        redoStack.clear();
    }

    graph::NodeId SequenceGraphEditorState::addEventNode(const autoinput::RecordedEvent& event,
                                                         std::optional<graph::NodeId> insertAfterNodeId)
    {
        pushUndoSnapshot();

        graph::NodeId precedingId = graph::InvalidNodeId;
        if (insertAfterNodeId.has_value() && graphDocument.findNode(*insertAfterNodeId) != nullptr)
        {
            precedingId = *insertAfterNodeId;
        }
        else if (viewerState.hasSelection() && graphDocument.findNode(viewerState.selectedNodeId) != nullptr)
        {
            const auto* selectedNode = graphDocument.findNode(viewerState.selectedNodeId);
            if (selectedNode->kind == graph::NodeKind::End)
            {
                auto chain = getLinearExecutionNodes(graphDocument);
                if (chain.size() >= 2)
                {
                    precedingId = chain[chain.size() - 2];
                }
                else
                {
                    precedingId = viewerState.selectedNodeId;
                }
            }
            else
            {
                precedingId = viewerState.selectedNodeId;
            }
        }
        else
        {
            auto chain = getLinearExecutionNodes(graphDocument);
            if (chain.size() >= 2)
            {
                precedingId = chain[chain.size() - 2];
            }
            else if (!chain.empty())
            {
                precedingId = chain.front();
            }
            else
            {
                for (const auto& n : graphDocument.nodes())
                {
                    if (n.kind == graph::NodeKind::Start)
                    {
                        precedingId = n.id;
                        break;
                    }
                }
            }
        }

        graph::NodeKind kind = (event.type == autoinput::RecordedEventType::Invalid) ? graph::NodeKind::Wait
                                                                                     : graph::NodeKind::RecordedEvent;

        std::string title = graph::formatRecordedEventTitle(event);
        std::string subtitle = graph::formatRecordedEventSubtitle(event, !adapterOptions.separateWaitNodes);

        auto& newNode = graphDocument.createNode(kind, title);
        newNode.setDetails(subtitle);
        graphDocument.createPin(newNode.id, graph::PinDirection::Input, "in");
        graphDocument.createPin(newNode.id, graph::PinDirection::Output, "out");

        if (precedingId != graph::InvalidNodeId)
        {
            const auto* prevOutPin = findPinOfDirection(graphDocument, precedingId, graph::PinDirection::Output);
            const auto* newInPin = findPinOfDirection(graphDocument, newNode.id, graph::PinDirection::Input);
            const auto* newOutPin = findPinOfDirection(graphDocument, newNode.id, graph::PinDirection::Output);

            if (prevOutPin != nullptr && newInPin != nullptr && newOutPin != nullptr)
            {
                graph::LinkId existingLinkId = graph::InvalidLinkId;
                graph::PinId oldTargetPinId = graph::InvalidPinId;

                for (const auto& link : graphDocument.links())
                {
                    if (link.fromPinId == prevOutPin->id)
                    {
                        existingLinkId = link.id;
                        oldTargetPinId = link.toPinId;
                        break;
                    }
                }

                if (existingLinkId != graph::InvalidLinkId)
                {
                    graphDocument.removeLink(existingLinkId);
                    graphDocument.createLink(prevOutPin->id, newInPin->id);
                    graphDocument.createLink(newOutPin->id, oldTargetPinId);
                }
                else
                {
                    graphDocument.createLink(prevOutPin->id, newInPin->id);
                    for (const auto& n : graphDocument.nodes())
                    {
                        if (n.kind == graph::NodeKind::End)
                        {
                            const auto* endInPin = findPinOfDirection(graphDocument, n.id, graph::PinDirection::Input);
                            if (endInPin != nullptr)
                            {
                                graphDocument.createLink(newOutPin->id, endInPin->id);
                            }
                            break;
                        }
                    }
                }
            }
        }

        autoLayout();
        viewerState.selectNode(newNode.id);
        validateCurrentGraph();
        statusMessage = std::format("Added node '{}' (ID #{}).", newNode.title, newNode.id);
        return newNode.id;
    }

    graph::NodeId SequenceGraphEditorState::addWaitNode(std::string_view delay,
                                                        std::optional<graph::NodeId> insertAfterNodeId)
    {
        autoinput::RecordedEvent waitEv{ .type = autoinput::RecordedEventType::Invalid, .delay = std::string(delay) };
        return addEventNode(waitEv, insertAfterNodeId);
    }

    bool SequenceGraphEditorState::deleteNode(graph::NodeId nodeId, bool reconnectChain)
    {
        const auto* node = graphDocument.findNode(nodeId);
        if (node == nullptr)
        {
            return false;
        }
        if (node->kind == graph::NodeKind::Start || node->kind == graph::NodeKind::End)
        {
            statusMessage = "Cannot delete required Start or End node.";
            return false;
        }

        pushUndoSnapshot();

        if (reconnectChain)
        {
            graph::PinId prevOutPinId = graph::InvalidPinId;
            graph::PinId nextInPinId = graph::InvalidPinId;

            for (graph::PinId pinId : node->pinIds)
            {
                const auto* pin = graphDocument.findPin(pinId);
                if (pin == nullptr)
                {
                    continue;
                }
                if (pin->direction == graph::PinDirection::Input)
                {
                    for (const auto& link : graphDocument.links())
                    {
                        if (link.toPinId == pin->id)
                        {
                            prevOutPinId = link.fromPinId;
                            break;
                        }
                    }
                }
                else if (pin->direction == graph::PinDirection::Output)
                {
                    for (const auto& link : graphDocument.links())
                    {
                        if (link.fromPinId == pin->id)
                        {
                            nextInPinId = link.toPinId;
                            break;
                        }
                    }
                }
            }

            graphDocument.removeNode(nodeId);

            if (prevOutPinId != graph::InvalidPinId && nextInPinId != graph::InvalidPinId)
            {
                graphDocument.createLink(prevOutPinId, nextInPinId);
            }
        }
        else
        {
            graphDocument.removeNode(nodeId);
        }

        if (viewerState.selectedNodeId == nodeId)
        {
            viewerState.clearSelection();
        }

        autoLayout();
        validateCurrentGraph();
        statusMessage = std::format("Deleted node #{} from graph.", nodeId);
        return true;
    }

    bool SequenceGraphEditorState::deleteSelectedNode(bool reconnectChain)
    {
        if (!viewerState.hasSelection())
        {
            return false;
        }
        return deleteNode(viewerState.selectedNodeId, reconnectChain);
    }

    bool SequenceGraphEditorState::updateNodeEvent(graph::NodeId nodeId, const autoinput::RecordedEvent& event)
    {
        auto* node = graphDocument.findNode(nodeId);
        if (node == nullptr || node->kind == graph::NodeKind::Start || node->kind == graph::NodeKind::End)
        {
            return false;
        }

        pushUndoSnapshot();

        node->kind = (event.type == autoinput::RecordedEventType::Invalid) ? graph::NodeKind::Wait
                                                                           : graph::NodeKind::RecordedEvent;
        node->title = graph::formatRecordedEventTitle(event);
        node->subtitle = graph::formatRecordedEventSubtitle(event, !adapterOptions.separateWaitNodes);
        node->sourceIndex = std::nullopt;

        validateCurrentGraph();
        statusMessage = std::format("Updated node #{} event parameters.", nodeId);
        return true;
    }

    bool SequenceGraphEditorState::updateNodeDelay(graph::NodeId nodeId, std::string_view newDelay)
    {
        const auto* node = graphDocument.findNode(nodeId);
        if (node == nullptr || node->kind == graph::NodeKind::Start || node->kind == graph::NodeKind::End)
        {
            return false;
        }

        auto event = graph::parseRecordedEventFromNode(*node);
        event.delay = std::string(newDelay);
        return updateNodeEvent(nodeId, event);
    }

    bool SequenceGraphEditorState::moveNodeUp(graph::NodeId nodeId)
    {
        const auto* node = graphDocument.findNode(nodeId);
        if (node == nullptr || node->kind == graph::NodeKind::Start || node->kind == graph::NodeKind::End)
        {
            return false;
        }

        auto chain = getLinearExecutionNodes(graphDocument);
        auto it = std::find(chain.begin(), chain.end(), nodeId);
        if (it == chain.end() || it == chain.begin() || it == chain.begin() + 1)
        {
            return false;
        }

        pushUndoSnapshot();

        auto idx = static_cast<std::size_t>(std::distance(chain.begin(), it));
        std::swap(chain[idx], chain[idx - 1]);

        auto allLinks = graphDocument.links();
        for (const auto& link : allLinks)
        {
            graphDocument.removeLink(link.id);
        }

        for (std::size_t i = 0; i + 1 < chain.size(); ++i)
        {
            const auto* outPin = findPinOfDirection(graphDocument, chain[i], graph::PinDirection::Output);
            const auto* inPin = findPinOfDirection(graphDocument, chain[i + 1], graph::PinDirection::Input);
            if (outPin != nullptr && inPin != nullptr)
            {
                graphDocument.createLink(outPin->id, inPin->id);
            }
        }

        autoLayout();
        validateCurrentGraph();
        statusMessage = std::format("Moved node #{} up.", nodeId);
        return true;
    }

    bool SequenceGraphEditorState::moveNodeDown(graph::NodeId nodeId)
    {
        const auto* node = graphDocument.findNode(nodeId);
        if (node == nullptr || node->kind == graph::NodeKind::Start || node->kind == graph::NodeKind::End)
        {
            return false;
        }

        auto chain = getLinearExecutionNodes(graphDocument);
        auto it = std::find(chain.begin(), chain.end(), nodeId);
        if (it == chain.end() || it + 1 >= chain.end() || *(it + 1) == chain.back())
        {
            return false;
        }

        pushUndoSnapshot();

        auto idx = static_cast<std::size_t>(std::distance(chain.begin(), it));
        std::swap(chain[idx], chain[idx + 1]);

        auto allLinks = graphDocument.links();
        for (const auto& link : allLinks)
        {
            graphDocument.removeLink(link.id);
        }

        for (std::size_t i = 0; i + 1 < chain.size(); ++i)
        {
            const auto* outPin = findPinOfDirection(graphDocument, chain[i], graph::PinDirection::Output);
            const auto* inPin = findPinOfDirection(graphDocument, chain[i + 1], graph::PinDirection::Input);
            if (outPin != nullptr && inPin != nullptr)
            {
                graphDocument.createLink(outPin->id, inPin->id);
            }
        }

        autoLayout();
        validateCurrentGraph();
        statusMessage = std::format("Moved node #{} down.", nodeId);
        return true;
    }

    bool SequenceGraphEditorState::reconnectLinearChain()
    {
        const graph::GraphNode* startNode = nullptr;
        const graph::GraphNode* endNode = nullptr;
        std::vector<const graph::GraphNode*> eventNodes;

        for (const auto& n : graphDocument.nodes())
        {
            if (n.kind == graph::NodeKind::Start && startNode == nullptr)
            {
                startNode = &n;
            }
            else if (n.kind == graph::NodeKind::End && endNode == nullptr)
            {
                endNode = &n;
            }
            else if (n.kind != graph::NodeKind::Start && n.kind != graph::NodeKind::End)
            {
                eventNodes.push_back(&n);
            }
        }

        if (startNode == nullptr || endNode == nullptr)
        {
            statusMessage = "Cannot reconnect linear chain: missing Start or End node.";
            return false;
        }

        pushUndoSnapshot();

        std::sort(eventNodes.begin(), eventNodes.end(),
                  [](const graph::GraphNode* a, const graph::GraphNode* b)
                  {
                      if (a->position.x != b->position.x)
                      {
                          return a->position.x < b->position.x;
                      }
                      return a->id < b->id;
                  });

        auto allLinks = graphDocument.links();
        for (const auto& link : allLinks)
        {
            graphDocument.removeLink(link.id);
        }

        std::vector<graph::NodeId> order;
        order.push_back(startNode->id);
        for (const auto* evNode : eventNodes)
        {
            order.push_back(evNode->id);
        }
        order.push_back(endNode->id);

        for (std::size_t i = 0; i + 1 < order.size(); ++i)
        {
            const auto* outPin = findPinOfDirection(graphDocument, order[i], graph::PinDirection::Output);
            const auto* inPin = findPinOfDirection(graphDocument, order[i + 1], graph::PinDirection::Input);
            if (outPin != nullptr && inPin != nullptr)
            {
                graphDocument.createLink(outPin->id, inPin->id);
            }
        }

        autoLayout();
        validateCurrentGraph();
        statusMessage = "Reconnected linear sequence chain.";
        return true;
    }

    bool SequenceGraphEditorState::connectNodes(graph::NodeId sourceNodeId, graph::NodeId targetNodeId)
    {
        const auto* outPin = findPinOfDirection(graphDocument, sourceNodeId, graph::PinDirection::Output);
        const auto* inPin = findPinOfDirection(graphDocument, targetNodeId, graph::PinDirection::Input);
        if (outPin == nullptr || inPin == nullptr)
        {
            return false;
        }

        pushUndoSnapshot();
        graphDocument.createLink(outPin->id, inPin->id);
        validateCurrentGraph();
        statusMessage = std::format("Connected node #{} to node #{}.", sourceNodeId, targetNodeId);
        return true;
    }

    void SequenceGraphEditorState::autoLayout(float startX, float startY, float stepX, float stepY)
    {
        auto chain = getLinearExecutionNodes(graphDocument);
        std::unordered_set<graph::NodeId> positioned;

        float curX = startX;
        float curY = startY;

        for (graph::NodeId id : chain)
        {
            auto* node = graphDocument.findNode(id);
            if (node != nullptr)
            {
                node->position = graph::NodePosition{ .x = curX, .y = curY };
                positioned.insert(id);
                curX += stepX;
                curY += stepY;
            }
        }

        float offX = startX;
        float offY = startY + 150.0F;
        for (auto& node : graphDocument.nodes())
        {
            if (!positioned.contains(node.id))
            {
                node.position = graph::NodePosition{ .x = offX, .y = offY };
                offX += stepX;
            }
        }
    }

    std::optional<SelectedNodeInspectionDetails> resolveNodeInspectionDetails(
        const graph::GraphDocument& doc, graph::NodeId nodeId, const autoinput::RecordedSequence& sequence,
        const graph::ValidationResult& validationResult)
    {
        const auto* node = doc.findNode(nodeId);
        if (node == nullptr)
        {
            return std::nullopt;
        }

        SelectedNodeInspectionDetails details;
        details.nodeId = node->id;
        details.kind = node->kind;
        details.title = node->title;
        details.subtitle = node->subtitle;
        details.sourceIndex = node->sourceIndex;

        if (details.sourceIndex.has_value() && *details.sourceIndex < sequence.events.size())
        {
            details.hasAssociatedEvent = true;
            details.associatedEvent = sequence.events[*details.sourceIndex];
        }
        else if (node->kind == graph::NodeKind::RecordedEvent || node->kind == graph::NodeKind::Wait)
        {
            details.hasAssociatedEvent = true;
            details.associatedEvent = graph::parseRecordedEventFromNode(*node);
        }

        details.validationIssues = graph::getNodeValidationIssues(validationResult, nodeId);
        return details;
    }

    const graph::GraphPin* findPinOfDirection(const graph::GraphDocument& doc, graph::NodeId nodeId,
                                              graph::PinDirection direction)
    {
        const auto* node = doc.findNode(nodeId);
        if (node == nullptr)
        {
            return nullptr;
        }
        for (graph::PinId pinId : node->pinIds)
        {
            const auto* pin = doc.findPin(pinId);
            if (pin != nullptr && pin->direction == direction)
            {
                return pin;
            }
        }
        return nullptr;
    }

    std::vector<graph::NodeId> getLinearExecutionNodes(const graph::GraphDocument& doc)
    {
        std::vector<graph::NodeId> result;
        const graph::GraphNode* startNode = nullptr;
        for (const auto& node : doc.nodes())
        {
            if (node.kind == graph::NodeKind::Start)
            {
                startNode = &node;
                break;
            }
        }
        if (startNode == nullptr)
        {
            return result;
        }

        std::unordered_set<graph::NodeId> visited;
        const graph::GraphNode* current = startNode;
        result.push_back(current->id);
        visited.insert(current->id);

        while (current != nullptr && current->kind != graph::NodeKind::End)
        {
            const graph::GraphLink* nextLink = nullptr;
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
                break;
            }
            const auto* toPin = doc.findPin(nextLink->toPinId);
            if (toPin == nullptr)
            {
                break;
            }
            const auto* nextNode = doc.findNode(toPin->nodeId);
            if (nextNode == nullptr || visited.contains(nextNode->id))
            {
                break;
            }
            visited.insert(nextNode->id);
            result.push_back(nextNode->id);
            current = nextNode;
        }
        return result;
    }

    std::string formatEventFieldsSummary(const autoinput::RecordedEvent& event)
    {
        switch (event.type)
        {
        case RecordedEventType::KeyDown:
            return std::format("Type: KeyDown | Key: \"{}\" | Delay: {}", event.key.value_or(""), event.delay);
        case RecordedEventType::KeyUp:
            return std::format("Type: KeyUp | Key: \"{}\" | Delay: {}", event.key.value_or(""), event.delay);
        case RecordedEventType::MouseDown:
            return std::format("Type: MouseDown | Button: \"{}\" | Delay: {}", event.button.value_or("left"),
                               event.delay);
        case RecordedEventType::MouseUp:
            return std::format("Type: MouseUp | Button: \"{}\" | Delay: {}", event.button.value_or("left"),
                               event.delay);
        case RecordedEventType::MouseMove:
            return std::format("Type: MouseMove | Pos: ({}, {}) | Delay: {}", event.x.value_or(0), event.y.value_or(0),
                               event.delay);
        case RecordedEventType::Invalid:
        default: return std::format("Type: Delay/Wait | Delay: {}", event.delay);
        }
    }

#ifdef AUTOINPUT_UI_INTERNAL_HAS_IMGUI
    namespace
    {
        void renderToolbar(autoinput::RecordedSequence& sequence, SequenceGraphEditorState& state, bool& modified)
        {
            ImGui::BeginGroup();

            if (ImGui::Button("Undo"))
            {
                state.undo();
            }
            ImGui::SameLine();
            if (ImGui::Button("Redo"))
            {
                state.redo();
            }
            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();

            if (ImGui::Button("+ Key"))
            {
                autoinput::RecordedEvent keyEv{ .type = autoinput::RecordedEventType::KeyDown,
                                                .delay = "10ms",
                                                .key = "space" };
                state.addEventNode(keyEv);
            }
            ImGui::SameLine();
            if (ImGui::Button("+ Mouse"))
            {
                autoinput::RecordedEvent mouseEv{ .type = autoinput::RecordedEventType::MouseDown,
                                                  .delay = "10ms",
                                                  .button = "left" };
                state.addEventNode(mouseEv);
            }
            ImGui::SameLine();
            if (ImGui::Button("+ Delay"))
            {
                state.addWaitNode("50ms");
            }
            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();

            const bool canDelete = state.viewerState.hasSelection();
            if (!canDelete)
            {
                ImGui::BeginDisabled();
            }
            if (ImGui::Button("Delete Node"))
            {
                state.deleteSelectedNode(true);
            }
            if (!canDelete)
            {
                ImGui::EndDisabled();
            }

            ImGui::SameLine();
            if (ImGui::Button("Move Up"))
            {
                if (state.viewerState.hasSelection())
                {
                    state.moveNodeUp(state.viewerState.selectedNodeId);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Move Down"))
            {
                if (state.viewerState.hasSelection())
                {
                    state.moveNodeDown(state.viewerState.selectedNodeId);
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Reconnect"))
            {
                state.reconnectLinearChain();
            }
            ImGui::SameLine();
            if (ImGui::Button("Auto Layout"))
            {
                state.autoLayout();
            }

            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();

            if (ImGui::Button("Rebuild Graph"))
            {
                state.rebuildFromSequence(sequence);
            }
            ImGui::SameLine();
            if (ImGui::Button("Validate"))
            {
                state.validateCurrentGraph();
            }
            ImGui::SameLine();

            if (ImGui::Button("Apply to Sequence"))
            {
                if (state.applyToSequence(sequence))
                {
                    modified = true;
                }
            }

            ImGui::SameLine();
            ImGui::TextDisabled("| View:");
            ImGui::SameLine();
            if (ImGui::RadioButton("Split", state.viewerState.viewMode == graph::FallbackGraphViewMode::Split))
            {
                state.viewerState.viewMode = graph::FallbackGraphViewMode::Split;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("List", state.viewerState.viewMode == graph::FallbackGraphViewMode::ListOnly))
            {
                state.viewerState.viewMode = graph::FallbackGraphViewMode::ListOnly;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Canvas", state.viewerState.viewMode == graph::FallbackGraphViewMode::Canvas))
            {
                state.viewerState.viewMode = graph::FallbackGraphViewMode::Canvas;
            }

            if (!state.statusMessage.empty())
            {
                ImGui::SameLine();
                const std::string statusStr = std::format("({})", state.statusMessage);
                ImGui::TextDisabled("%s", statusStr.c_str());
            }
            ImGui::EndGroup();
        }

        void renderInspectorPanel(const autoinput::RecordedSequence& sequence, SequenceGraphEditorState& state)
        {
            ImGui::TextUnformatted("Selected Node Inspector");
            ImGui::Separator();

            if (state.viewerState.selectedNodeId == graph::InvalidNodeId)
            {
                ImGui::TextDisabled("Select a node from the canvas or list to inspect its properties.");
                return;
            }

            auto details = state.getSelectedNodeDetails(sequence);
            if (!details.has_value())
            {
                return;
            }

            const std::string idText = std::format("Node ID: #{}", details->nodeId);
            const std::string kindText = std::format("Kind: {}", graph::nodeKindToString(details->kind));
            const std::string titleText = std::format("Title: {}", details->title);
            ImGui::TextUnformatted(idText.c_str());
            ImGui::TextUnformatted(kindText.c_str());
            ImGui::TextUnformatted(titleText.c_str());

            if (!details->subtitle.empty())
            {
                const std::string detailsText = std::format("Details: {}", details->subtitle);
                ImGui::TextUnformatted(detailsText.c_str());
            }

            if (details->kind == graph::NodeKind::RecordedEvent || details->kind == graph::NodeKind::Wait)
            {
                ImGui::Spacing();
                ImGui::SeparatorText("Node Parameters");

                auto ev = details->associatedEvent;
                bool evChanged = false;

                // Event Type Combo
                const char* typeNames[] = { "KeyDown", "KeyUp", "MouseDown", "MouseUp", "MouseMove", "Delay/Wait" };
                int currentTypeIdx = 5;
                if (ev.type == autoinput::RecordedEventType::KeyDown)
                {
                    currentTypeIdx = 0;
                }
                else if (ev.type == autoinput::RecordedEventType::KeyUp)
                {
                    currentTypeIdx = 1;
                }
                else if (ev.type == autoinput::RecordedEventType::MouseDown)
                {
                    currentTypeIdx = 2;
                }
                else if (ev.type == autoinput::RecordedEventType::MouseUp)
                {
                    currentTypeIdx = 3;
                }
                else if (ev.type == autoinput::RecordedEventType::MouseMove)
                {
                    currentTypeIdx = 4;
                }

                if (ImGui::Combo("Event Type", &currentTypeIdx, typeNames, 6))
                {
                    switch (currentTypeIdx)
                    {
                    case 0: ev.type = autoinput::RecordedEventType::KeyDown; break;
                    case 1: ev.type = autoinput::RecordedEventType::KeyUp; break;
                    case 2: ev.type = autoinput::RecordedEventType::MouseDown; break;
                    case 3: ev.type = autoinput::RecordedEventType::MouseUp; break;
                    case 4: ev.type = autoinput::RecordedEventType::MouseMove; break;
                    case 5:
                    default: ev.type = autoinput::RecordedEventType::Invalid; break;
                    }
                    evChanged = true;
                }

                // Delay input
                char delayBuf[64]{};
                std::strncpy(delayBuf, ev.delay.c_str(), sizeof(delayBuf) - 1);
                if (ImGui::InputText("Delay", delayBuf, sizeof(delayBuf)))
                {
                    ev.delay = delayBuf;
                    evChanged = true;
                }

                // Key input for KeyDown / KeyUp
                if (ev.type == autoinput::RecordedEventType::KeyDown || ev.type == autoinput::RecordedEventType::KeyUp)
                {
                    char keyBuf[64]{};
                    if (ev.key.has_value())
                    {
                        std::strncpy(keyBuf, ev.key->c_str(), sizeof(keyBuf) - 1);
                    }
                    if (ImGui::InputText("Key", keyBuf, sizeof(keyBuf)))
                    {
                        ev.key = std::string(keyBuf);
                        evChanged = true;
                    }
                }

                // Button input for MouseDown / MouseUp
                if (ev.type == autoinput::RecordedEventType::MouseDown ||
                    ev.type == autoinput::RecordedEventType::MouseUp)
                {
                    const char* btnNames[] = { "left", "right", "middle" };
                    int btnIdx = 0;
                    if (ev.button.has_value())
                    {
                        if (*ev.button == "right")
                        {
                            btnIdx = 1;
                        }
                        else if (*ev.button == "middle")
                        {
                            btnIdx = 2;
                        }
                    }
                    if (ImGui::Combo("Button", &btnIdx, btnNames, 3))
                    {
                        ev.button = btnNames[btnIdx];
                        evChanged = true;
                    }
                }

                // Coordinates for MouseMove
                if (ev.type == autoinput::RecordedEventType::MouseMove)
                {
                    int pos[2] = { ev.x.value_or(0), ev.y.value_or(0) };
                    if (ImGui::InputInt2("Position (X, Y)", pos))
                    {
                        ev.x = pos[0];
                        ev.y = pos[1];
                        evChanged = true;
                    }
                }

                if (evChanged)
                {
                    state.updateNodeEvent(details->nodeId, ev);
                }

                ImGui::Spacing();
                if (ImGui::Button("Delete Node"))
                {
                    state.deleteNode(details->nodeId, true);
                }
                ImGui::SameLine();
                if (ImGui::Button("Move Up"))
                {
                    state.moveNodeUp(details->nodeId);
                }
                ImGui::SameLine();
                if (ImGui::Button("Move Down"))
                {
                    state.moveNodeDown(details->nodeId);
                }
            }

            if (!details->validationIssues.empty())
            {
                ImGui::Spacing();
                ImGui::SeparatorText("Node Issues");
                for (const auto& issue : details->validationIssues)
                {
                    const std::string msg =
                        std::format("[{}] {}", graph::validationSeverityToString(issue.severity), issue.message);
                    if (issue.severity == graph::ValidationSeverity::Error)
                    {
                        ImGui::TextColored(ImVec4(0.95F, 0.3F, 0.3F, 1.0F), "%s", msg.c_str());
                    }
                    else if (issue.severity == graph::ValidationSeverity::Warning)
                    {
                        ImGui::TextColored(ImVec4(0.95F, 0.8F, 0.2F, 1.0F), "%s", msg.c_str());
                    }
                    else
                    {
                        ImGui::TextColored(ImVec4(0.4F, 0.7F, 1.0F, 1.0F), "%s", msg.c_str());
                    }
                }
            }
        }

        void renderValidationPanel(const autoinput::RecordedSequence& sequence, SequenceGraphEditorState& state)
        {
            ImGui::TextUnformatted("Graph Validation");
            if (state.validationResult.isValid())
            {
                ImGui::TextColored(ImVec4(0.3F, 0.9F, 0.3F, 1.0F), "Graph topology is valid linear sequence.");
                if (state.validationResult.hasWarnings())
                {
                    ImGui::TextColored(ImVec4(0.95F, 0.8F, 0.2F, 1.0F), "%zu warning(s) present.",
                                       state.validationResult.warningCount());
                }
            }
            else
            {
                ImGui::TextColored(ImVec4(0.95F, 0.3F, 0.3F, 1.0F), "Validation failed: %zu error(s).",
                                   state.validationResult.errorCount());
            }

            if (state.lastCompilationError.has_value())
            {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.95F, 0.2F, 0.2F, 1.0F), "Compilation Error: %s",
                                   state.lastCompilationError->c_str());
            }

            if (!state.validationResult.issues.empty())
            {
                ImGui::Spacing();
                ImGui::BeginChild("ValidationIssuesList", ImVec2(0, 100.0F), true);
                for (const auto& issue : state.validationResult.issues)
                {
                    const std::string issueText =
                        std::format("[{}] {}", graph::validationSeverityToString(issue.severity), issue.message);
                    if (issue.nodeId.has_value())
                    {
                        const std::string buttonLabel = std::format("Select #{}", *issue.nodeId);
                        if (ImGui::SmallButton(buttonLabel.c_str()))
                        {
                            state.viewerState.selectNode(*issue.nodeId);
                        }
                        ImGui::SameLine();
                    }
                    ImGui::TextUnformatted(issueText.c_str());
                }
                ImGui::EndChild();
            }
        }
    } // namespace
#endif

    bool renderSequenceGraphEditor(autoinput::RecordedSequence& sequence, SequenceGraphEditorState& state,
                                   const char* editorId)
    {
        state.syncWithSequence(sequence);

        bool sequenceModified = false;

#ifdef AUTOINPUT_UI_INTERNAL_HAS_IMGUI
        ImGui::PushID(editorId);

        renderToolbar(sequence, state, sequenceModified);
        ImGui::Separator();

        float inspectorWidth = 320.0F;
        float availWidth = ImGui::GetContentRegionAvail().x;
        float mainViewerWidth = (availWidth > inspectorWidth + 100.0F) ? (availWidth - inspectorWidth - 10.0F) : 0.0F;

        if (mainViewerWidth > 0.0F)
        {
            ImGui::BeginChild("MainGraphViewerRegion", ImVec2(mainViewerWidth, 0), false);
            graph::renderFallbackGraphViewer(state.graphDocument, state.validationResult, state.viewerState,
                                             "SequenceGraphCanvas");
            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("SideInspectorRegion", ImVec2(inspectorWidth, 0), true);
            renderInspectorPanel(sequence, state);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            renderValidationPanel(sequence, state);
            ImGui::EndChild();
        }
        else
        {
            graph::renderFallbackGraphViewer(state.graphDocument, state.validationResult, state.viewerState,
                                             "SequenceGraphCanvas");
            ImGui::Separator();
            renderInspectorPanel(sequence, state);
            ImGui::Separator();
            renderValidationPanel(sequence, state);
        }

        ImGui::PopID();
#endif

        return sequenceModified;
    }

} // namespace autoinput::ui::editors
