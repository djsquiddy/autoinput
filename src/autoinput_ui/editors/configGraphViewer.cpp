/**
 * @file configGraphViewer.cpp
 * @brief Implementation of read-only visual graph viewer and diagnostics inspector for ConfigData.
 * @author djsquiddy
 * @date August 2026
 */
#include "configGraphViewer.h"

#include "autoinput/config/configValidator.h"

#include <algorithm>
#include <cctype>
#include <format>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#if __has_include(<imgui.h>)
#include <imgui.h>
#define AUTOINPUT_UI_INTERNAL_HAS_IMGUI 1
#endif

namespace autoinput::ui::editors
{
    std::optional<ConfigNodeInspectionDetails> inspectConfigGraphNode(const autoinput::ConfigData& config,
                                                                      const graph::GraphDocument& doc,
                                                                      graph::NodeId nodeId,
                                                                      const ConfigDiagnosticsResult* diagnostics)
    {
        const auto* node = doc.findNode(nodeId);
        if (node == nullptr)
        {
            return std::nullopt;
        }

        ConfigNodeInspectionDetails details;
        details.nodeId = node->id;
        details.kind = node->kind;
        details.title = node->title;
        details.subtitle = node->subtitle;
        details.sourceIndex = node->sourceIndex;

        switch (node->kind)
        {
        case graph::NodeKind::Command:
            if (node->sourceIndex.has_value() && *node->sourceIndex < config.commands.size())
            {
                const auto& cmd = config.commands[*node->sourceIndex];
                details.commandIndex = node->sourceIndex;

                for (const auto& sk : cmd.startKeys)
                    details.connectedInputs.push_back(std::format("Start: {}", sk));
                for (const auto& k : cmd.keys)
                    details.connectedInputs.push_back(std::format("Key: {}", k));
                for (const auto& b : cmd.buttons)
                    details.connectedInputs.push_back(std::format("Button: {}", b));
                for (const auto& c : cmd.controls)
                    details.connectedControls.push_back(std::format("{} -> {}", c.action, c.input));
                if (!cmd.exclusiveGroup.empty()) details.connectedGroups.push_back(cmd.exclusiveGroup);
            }
            break;

        case graph::NodeKind::Control:
            details.controlIndex = node->sourceIndex;
            // Find incoming command link
            for (const auto pinId : node->pinIds)
            {
                const auto* pin = doc.findPin(pinId);
                if (pin != nullptr && pin->direction == graph::PinDirection::Input)
                {
                    for (const auto& link : doc.links())
                    {
                        if (link.toPinId == pin->id)
                        {
                            const auto* fromPin = doc.findPin(link.fromPinId);
                            if (fromPin != nullptr)
                            {
                                const auto* fromNode = doc.findNode(fromPin->nodeId);
                                if (fromNode != nullptr && fromNode->kind == graph::NodeKind::Command)
                                {
                                    details.commandIndex = fromNode->sourceIndex;
                                    details.connectedTargets.push_back(fromNode->title);
                                }
                            }
                        }
                    }
                }
            }
            break;

        case graph::NodeKind::Sequence:
            if (node->sourceIndex.has_value() && *node->sourceIndex < config.sequences.size())
            {
                const auto& seq = config.sequences[*node->sourceIndex];
                details.sequenceIndex = node->sourceIndex;
                if (!seq.start.empty())
                {
                    details.connectedInputs.push_back(std::format("Start: {}", seq.start));
                }
                details.connectedTargets.push_back(std::format("{} events", seq.events.size()));
            }
            break;

        case graph::NodeKind::ExclusiveGroup:
            // Find all connected member commands
            for (const auto& link : doc.links())
            {
                const auto* toPin = doc.findPin(link.toPinId);
                if (toPin != nullptr && toPin->nodeId == node->id)
                {
                    const auto* fromPin = doc.findPin(link.fromPinId);
                    if (fromPin != nullptr)
                    {
                        const auto* fromNode = doc.findNode(fromPin->nodeId);
                        if (fromNode != nullptr)
                        {
                            details.connectedTargets.push_back(fromNode->title);
                        }
                    }
                }
            }
            break;

        case graph::NodeKind::ApplicationFilter:
        case graph::NodeKind::BlacklistEntry:
            // Find target commands/sequences linked from this filter
            for (const auto& link : doc.links())
            {
                const auto* fromPin = doc.findPin(link.fromPinId);
                if (fromPin != nullptr && fromPin->nodeId == node->id)
                {
                    const auto* toPin = doc.findPin(link.toPinId);
                    if (toPin != nullptr)
                    {
                        const auto* toNode = doc.findNode(toPin->nodeId);
                        if (toNode != nullptr)
                        {
                            details.connectedTargets.push_back(toNode->title);
                        }
                    }
                }
            }
            break;

        case graph::NodeKind::Input:
            // Find targets that this input triggers
            for (const auto& link : doc.links())
            {
                const auto* fromPin = doc.findPin(link.fromPinId);
                if (fromPin != nullptr && fromPin->nodeId == node->id)
                {
                    const auto* toPin = doc.findPin(link.toPinId);
                    if (toPin != nullptr)
                    {
                        const auto* toNode = doc.findNode(toPin->nodeId);
                        if (toNode != nullptr)
                        {
                            details.connectedTargets.push_back(toNode->title);
                        }
                    }
                }
            }
            break;

        default: break;
        }

        // Correlate diagnostics
        if (diagnostics != nullptr)
        {
            for (const auto& issue : diagnostics->issues)
            {
                bool matches = false;
                if (issue.associatedNodeId.has_value() && *issue.associatedNodeId == node->id)
                {
                    matches = true;
                }
                else if (details.commandIndex.has_value() && issue.commandIndex == details.commandIndex)
                {
                    matches = true;
                }
                else if (details.sequenceIndex.has_value() && issue.sequenceIndex == details.sequenceIndex)
                {
                    matches = true;
                }

                if (matches)
                {
                    details.diagnosticIssues.push_back(issue);
                }
            }
        }

        return details;
    }

    void ConfigGraphViewerState::syncWithConfig(const autoinput::ConfigData& config, bool force)
    {
        if (!isGraphSynchronized || force || config.commands.size() != cachedCommandCount ||
            config.sequences.size() != cachedSequenceCount || config.application != cachedAppName ||
            config.endKey != cachedEndKey)
        {
            rebuildFromConfig(config);
        }
    }

    void ConfigGraphViewerState::rebuildFromConfig(const autoinput::ConfigData& config)
    {
        // Update adapter options according to filter toggles
        adapterOptions.includeCommands = showCommands;
        adapterOptions.includeControls = showControls;
        adapterOptions.includeSequences = showSequences;
        adapterOptions.includeInputs = showInputs;
        adapterOptions.includeExclusiveGroups = showExclusiveGroups;
        adapterOptions.includeGlobalSettings = showGlobalSettings;
        adapterOptions.includeApplicationFilter = showApplicationFilter;
        adapterOptions.includeBlacklist = showBlacklist;

        graphDocument = graph::configToGraphDocument(config, adapterOptions);
        runDiagnostics(config);

        cachedCommandCount = config.commands.size();
        cachedSequenceCount = config.sequences.size();
        cachedAppName = config.application;
        cachedEndKey = config.endKey;
        isGraphSynchronized = true;

        statusMessage = std::format("Config graph ready ({} nodes, {} links).", graphDocument.nodeCount(),
                                    graphDocument.linkCount());
    }

    void ConfigGraphViewerState::runDiagnostics(const autoinput::ConfigData& config)
    {
        diagnosticsResult = analyzeConfigDiagnostics(config, &graphDocument);
    }

    void ConfigGraphViewerState::selectNode(graph::NodeId nodeId)
    {
        viewerState.selectNode(nodeId);
    }

    void ConfigGraphViewerState::clearSelection()
    {
        viewerState.clearSelection();
    }

    graph::NodeId ConfigGraphViewerState::getSelectedNodeId() const noexcept
    {
        return viewerState.selectedNodeId;
    }

    bool ConfigGraphViewerState::hasSelection() const noexcept
    {
        return viewerState.hasSelection();
    }

#if defined(AUTOINPUT_UI_INTERNAL_HAS_IMGUI)
    namespace
    {
        bool renderConfigFilterToolbar(const autoinput::ConfigData& config, ConfigGraphViewerState& state)
        {
            bool filterChanged = false;
            bool stateChanged = false;

            ImGui::TextUnformatted("Filter:");
            ImGui::SameLine();
            if (ImGui::Checkbox("Commands", &state.showCommands))
            {
                filterChanged = true;
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("Controls", &state.showControls))
            {
                filterChanged = true;
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("Sequences", &state.showSequences))
            {
                filterChanged = true;
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("Inputs", &state.showInputs))
            {
                filterChanged = true;
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("Groups", &state.showExclusiveGroups))
            {
                filterChanged = true;
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("Global", &state.showGlobalSettings))
            {
                filterChanged = true;
            }

            if (state.showGlobalSettings)
            {
                ImGui::SameLine();
                if (ImGui::Checkbox("App Filter", &state.showApplicationFilter))
                {
                    filterChanged = true;
                }
                ImGui::SameLine();
                if (ImGui::Checkbox("Blacklist", &state.showBlacklist))
                {
                    filterChanged = true;
                }
            }

            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();

            if (ImGui::Button("Refresh"))
            {
                state.rebuildFromConfig(config);
                stateChanged = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear Selection") && state.hasSelection())
            {
                state.clearSelection();
                stateChanged = true;
            }

            ImGui::SameLine();
            if (state.diagnosticsResult.hasErrors())
            {
                ImGui::TextColored(ImVec4(0.95F, 0.3F, 0.3F, 1.0F), "[Errors: %zu]",
                                   state.diagnosticsResult.errorCount());
            }
            else if (state.diagnosticsResult.hasWarnings())
            {
                ImGui::TextColored(ImVec4(0.95F, 0.75F, 0.2F, 1.0F), "[Warnings: %zu]",
                                   state.diagnosticsResult.warningCount());
            }
            else
            {
                ImGui::TextColored(ImVec4(0.3F, 0.85F, 0.4F, 1.0F), "[Clean]");
            }

            if (filterChanged)
            {
                state.rebuildFromConfig(config);
                stateChanged = true;
            }

            ImGui::Separator();
            return stateChanged;
        }

        void renderConfigNodeInspectorPanel(const autoinput::ConfigData& config, const ConfigGraphViewerState& state)
        {
            ImGui::TextUnformatted("Node Inspector");
            ImGui::Separator();

            if (state.viewerState.selectedNodeId != graph::InvalidNodeId)
            {
                const auto details = inspectConfigGraphNode(config, state.graphDocument,
                                                            state.viewerState.selectedNodeId, &state.diagnosticsResult);
                if (details.has_value())
                {
                    const std::string kindStr(graph::nodeKindToString(details->kind));
                    ImGui::Text("Kind: %s", kindStr.c_str());
                    ImGui::Text("Title: %s", details->title.c_str());
                    if (!details->subtitle.empty())
                    {
                        ImGui::TextWrapped("Details: %s", details->subtitle.c_str());
                    }
                    if (details->sourceIndex.has_value())
                    {
                        ImGui::Text("Source Index: #%zu", *details->sourceIndex);
                    }

                    if (!details->connectedInputs.empty())
                    {
                        ImGui::Spacing();
                        ImGui::TextDisabled("Inputs:");
                        for (const auto& inText : details->connectedInputs)
                        {
                            ImGui::BulletText("%s", inText.c_str());
                        }
                    }

                    if (!details->connectedControls.empty())
                    {
                        ImGui::Spacing();
                        ImGui::TextDisabled("Controls:");
                        for (const auto& ctrlText : details->connectedControls)
                        {
                            ImGui::BulletText("%s", ctrlText.c_str());
                        }
                    }

                    if (!details->connectedGroups.empty())
                    {
                        ImGui::Spacing();
                        ImGui::TextDisabled("Exclusive Groups:");
                        for (const auto& grpText : details->connectedGroups)
                        {
                            ImGui::BulletText("%s", grpText.c_str());
                        }
                    }

                    if (!details->connectedTargets.empty())
                    {
                        ImGui::Spacing();
                        ImGui::TextDisabled("Connected Targets:");
                        for (const auto& tgtText : details->connectedTargets)
                        {
                            ImGui::BulletText("%s", tgtText.c_str());
                        }
                    }

                    if (!details->diagnosticIssues.empty())
                    {
                        ImGui::Spacing();
                        ImGui::TextColored(ImVec4(0.95F, 0.4F, 0.3F, 1.0F),
                                           "Diagnostics (%zu):", details->diagnosticIssues.size());
                        for (const auto& issue : details->diagnosticIssues)
                        {
                            const std::string sevStr(configDiagnosticSeverityToString(issue.severity));
                            ImGui::BulletText("[%s] %s", sevStr.c_str(), issue.message.c_str());
                        }
                    }
                }
            }
            else
            {
                ImGui::TextDisabled("Select a node on the graph to inspect properties and connections.");
            }
            ImGui::Spacing();
            ImGui::Separator();
        }

        bool renderConfigDiagnosticsPanel(ConfigGraphViewerState& state)
        {
            bool stateChanged = false;
            ImGui::Text("Configuration Diagnostics (%zu)", state.diagnosticsResult.issues.size());
            ImGui::Separator();

            if (state.diagnosticsResult.issues.empty())
            {
                ImGui::TextColored(ImVec4(0.3F, 0.85F, 0.4F, 1.0F), "No configuration issues detected.");
            }
            else
            {
                ImGui::BeginChild("DiagnosticsListRegion", ImVec2(0.0F, 0.0F), false);
                for (std::size_t i = 0; i < state.diagnosticsResult.issues.size(); ++i)
                {
                    const auto& issue = state.diagnosticsResult.issues[i];
                    ImGui::PushID(static_cast<int>(i));

                    ImVec4 color(0.35F, 0.7F, 0.95F, 1.0F);
                    if (issue.severity == ConfigDiagnosticSeverity::Error)
                    {
                        color = ImVec4(0.95F, 0.3F, 0.3F, 1.0F);
                    }
                    else if (issue.severity == ConfigDiagnosticSeverity::Warning)
                    {
                        color = ImVec4(0.95F, 0.75F, 0.2F, 1.0F);
                    }

                    const std::string sevStr(configDiagnosticSeverityToString(issue.severity));
                    ImGui::TextColored(color, "[%s] %s", sevStr.c_str(), issue.category.c_str());
                    ImGui::TextWrapped("%s", issue.message.c_str());

                    if (issue.relatedInput.has_value() && !issue.relatedInput->empty())
                    {
                        ImGui::TextDisabled("Related Input: '%s'", issue.relatedInput->c_str());
                    }

                    if (!issue.suggestedFix.empty())
                    {
                        ImGui::TextDisabled("Fix: %s", issue.suggestedFix.c_str());
                    }

                    if (issue.associatedNodeId.has_value())
                    {
                        if (ImGui::SmallButton("Select Node"))
                        {
                            state.selectNode(*issue.associatedNodeId);
                            stateChanged = true;
                        }
                    }

                    ImGui::Separator();
                    ImGui::PopID();
                }
                ImGui::EndChild();
            }
            return stateChanged;
        }
    } // namespace
#endif

    bool renderConfigGraphViewer(const autoinput::ConfigData& config, ConfigGraphViewerState& state,
                                 const char* viewerId)
    {
        state.syncWithConfig(config);

#if defined(AUTOINPUT_UI_INTERNAL_HAS_IMGUI)
        bool stateChanged = false;

        ImGui::PushID(viewerId);

        if (state.showFilterToolbar)
        {
            if (renderConfigFilterToolbar(config, state))
            {
                stateChanged = true;
            }
        }

        const float inspectorWidth = (state.showInspectorPanel || state.showDiagnosticsPanel) ? 340.0F : 0.0F;
        const float canvasWidth =
            ImGui::GetContentRegionAvail().x - (inspectorWidth > 0.0F ? inspectorWidth + 10.0F : 0.0F);

        ImGui::BeginChild("ConfigGraphCanvasChild", ImVec2(canvasWidth, 0.0F), true);
        {
            graph::renderFallbackGraphViewer(state.graphDocument, state.viewerState,
                                             graph::ValidationOptions::configGraph(), "ConfigGraphCanvas");
        }
        ImGui::EndChild();

        if (inspectorWidth > 0.0F)
        {
            ImGui::SameLine();
            ImGui::BeginChild("ConfigGraphInspectorSidePanel", ImVec2(inspectorWidth, 0.0F), true);
            {
                if (state.showInspectorPanel)
                {
                    renderConfigNodeInspectorPanel(config, state);
                }

                if (state.showDiagnosticsPanel)
                {
                    if (renderConfigDiagnosticsPanel(state))
                    {
                        stateChanged = true;
                    }
                }
            }
            ImGui::EndChild();
        }

        ImGui::PopID();
        return stateChanged;
#else
        return false;
#endif
    }

} // namespace autoinput::ui::editors
