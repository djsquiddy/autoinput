/**
 * @file sequenceGraphEditor.cpp
 * @brief Implementation of sequence graph editor and inspection UI component.
 * @author djsquiddy
 * @date August 2026
 */
#include "sequenceGraphEditor.h"

#include <format>
#include <string_view>

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

        validateCurrentGraph();
        isGraphSynchronized = true;
        statusMessage = std::format("Graph synchronized with sequence ({} nodes).", graphDocument.nodeCount());
    }

    bool SequenceGraphEditorState::validateCurrentGraph()
    {
        validationResult = graph::validateGraph(graphDocument, graph::ValidationOptions::sequenceGraph());
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
        auto result = compileGraph(compileOptions.sourceSequence);
        if (result.success && result.sequence.has_value())
        {
            targetSequence = std::move(*result.sequence);
            cachedSequenceEventCount = targetSequence.events.size();
            isGraphSynchronized = true;
            statusMessage = "Graph successfully compiled and applied to sequence.";
            return true;
        }
        return false;
    }

    std::optional<SelectedNodeInspectionDetails> SequenceGraphEditorState::getSelectedNodeDetails(
        const autoinput::RecordedSequence& sequence) const
    {
        return resolveNodeInspectionDetails(graphDocument, viewerState.selectedNodeId, sequence, validationResult);
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

        details.validationIssues = graph::getNodeValidationIssues(validationResult, nodeId);
        return details;
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

            if (state.isEditingAllowed)
            {
                if (ImGui::Button("Apply to Sequence"))
                {
                    if (state.applyToSequence(sequence))
                    {
                        modified = true;
                    }
                }
            }
            else
            {
                ImGui::BeginDisabled();
                ImGui::Button("Apply to Sequence");
                ImGui::EndDisabled();
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                {
                    ImGui::SetTooltip(
                        "Graph editing is currently in safe inspection mode. Edit sequence in the Steps Table.");
                }
            }

            ImGui::SameLine();
            if (ImGui::Checkbox("Separate Wait Nodes", &state.adapterOptions.separateWaitNodes))
            {
                state.rebuildFromSequence(sequence);
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

        void renderInspectorPanel(const autoinput::RecordedSequence& sequence, const SequenceGraphEditorState& state)
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

            if (details->sourceIndex.has_value())
            {
                ImGui::Spacing();
                ImGui::SeparatorText("Recorded Event Mapping");
                const std::string sourceIdxText = std::format("Source Index: Event #{}", *details->sourceIndex);
                ImGui::BulletText("%s", sourceIdxText.c_str());

                if (details->hasAssociatedEvent)
                {
                    const auto& ev = details->associatedEvent;
                    const std::string summaryText = std::format("Event Summary: {}", formatEventFieldsSummary(ev));
                    const std::string delayText = std::format("Delay: {}", ev.delay);
                    ImGui::BulletText("%s", summaryText.c_str());
                    ImGui::BulletText("%s", delayText.c_str());

                    if (ev.key.has_value())
                    {
                        const std::string keyText = std::format("Key: \"{}\"", *ev.key);
                        ImGui::BulletText("%s", keyText.c_str());
                    }
                    if (ev.button.has_value())
                    {
                        const std::string btnText = std::format("Mouse Button: \"{}\"", *ev.button);
                        ImGui::BulletText("%s", btnText.c_str());
                    }
                    if (ev.x.has_value() || ev.y.has_value())
                    {
                        const std::string posText =
                            std::format("Position: ({}, {})", ev.x.value_or(0), ev.y.value_or(0));
                        ImGui::BulletText("%s", posText.c_str());
                    }
                }
                else
                {
                    ImGui::TextDisabled("Source event index points outside current sequence events.");
                }

                ImGui::Spacing();
                ImGui::TextDisabled("Safe inspection mode: use Steps Table to modify parameters.");
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
                ImGui::TextColored(ImVec4(0.2F, 0.85F, 0.3F, 1.0F), "Topology Valid");
                const std::string flowText =
                    std::format("Linear flow: Start -> {} event(s) -> End", sequence.events.size());
                ImGui::TextDisabled("%s", flowText.c_str());
            }
            else
            {
                ImGui::TextColored(ImVec4(0.95F, 0.3F, 0.3F, 1.0F), "Issues Detected (%zu)",
                                   state.validationResult.issues.size());

                for (std::size_t i = 0; i < state.validationResult.issues.size(); ++i)
                {
                    const auto& issue = state.validationResult.issues[i];
                    const std::string issueText =
                        std::format("[{}] {}", graph::validationSeverityToString(issue.severity), issue.message);

                    ImGui::BulletText("%s", issueText.c_str());

                    if (issue.nodeId.has_value())
                    {
                        ImGui::SameLine();
                        const std::string btnLabel = std::format("Select #{}##issue_{}", *issue.nodeId, i);
                        if (ImGui::SmallButton(btnLabel.c_str()))
                        {
                            state.viewerState.selectNode(*issue.nodeId);
                        }
                    }
                }
            }
        }
    } // namespace
#endif

    bool renderSequenceGraphEditor(autoinput::RecordedSequence& sequence, SequenceGraphEditorState& state,
                                   const char* editorId)
    {
#ifdef AUTOINPUT_UI_INTERNAL_HAS_IMGUI
        if (ImGui::GetCurrentContext() == nullptr)
        {
            return false;
        }

        state.syncWithSequence(sequence);

        bool modified = false;
        ImGui::PushID(editorId);

        renderToolbar(sequence, state, modified);
        ImGui::Separator();

        const float availableWidth = ImGui::GetContentRegionAvail().x;
        const float leftPanelWidth = (availableWidth > 500.0F) ? (availableWidth * 0.62F) : availableWidth;

        // Left Panel: Fallback Graph Viewer
        ImGui::BeginChild("GraphViewerArea", ImVec2(leftPanelWidth, 0), true);
        graph::renderFallbackGraphViewer(state.graphDocument, state.validationResult, state.viewerState,
                                         "SeqGraphViewer");
        ImGui::EndChild();

        // Right Panel: Inspector & Validation Side Panel
        if (availableWidth > 500.0F)
        {
            ImGui::SameLine();
            ImGui::BeginChild("GraphInspectorSideArea", ImVec2(0, 0), true);
            renderInspectorPanel(sequence, state);
            ImGui::Spacing();
            ImGui::Separator();
            renderValidationPanel(sequence, state);
            ImGui::EndChild();
        }

        ImGui::PopID();
        return modified;
#else
        (void)sequence;
        (void)state;
        (void)editorId;
        return false;
#endif
    }

} // namespace autoinput::ui::editors
