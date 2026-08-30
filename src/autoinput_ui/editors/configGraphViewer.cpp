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
    namespace
    {
        [[nodiscard]] std::string toLowerTrimmed(std::string_view str)
        {
            std::string result;
            result.reserve(str.size());
            for (char ch : str)
            {
                if (!std::isspace(static_cast<unsigned char>(ch)))
                {
                    result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
                }
            }
            return result;
        }

        [[nodiscard]] std::string getCommandDisplayName(const autoinput::CommandData& cmd, std::size_t index)
        {
            if (!cmd.name.empty())
            {
                return cmd.name;
            }
            return std::format("Command {}", index + 1);
        }

        [[nodiscard]] std::string getSequenceDisplayName(const autoinput::RecordedSequence& seq, std::size_t index)
        {
            if (!seq.name.empty())
            {
                return seq.name;
            }
            return std::format("Sequence {}", index + 1);
        }

        void mapIssuesToGraphNodes(ConfigDiagnosticsResult& result, const graph::GraphDocument& doc)
        {
            for (auto& issue : result.issues)
            {
                if (issue.associatedNodeId.has_value())
                {
                    continue;
                }

                if (issue.commandIndex.has_value())
                {
                    for (const auto& node : doc.nodes())
                    {
                        if (node.kind == graph::NodeKind::Command && node.sourceIndex == issue.commandIndex)
                        {
                            issue.associatedNodeId = node.id;
                            break;
                        }
                    }
                }
                else if (issue.sequenceIndex.has_value())
                {
                    for (const auto& node : doc.nodes())
                    {
                        if (node.kind == graph::NodeKind::Sequence && node.sourceIndex == issue.sequenceIndex)
                        {
                            issue.associatedNodeId = node.id;
                            break;
                        }
                    }
                }
            }
        }
    } // namespace

    ConfigDiagnosticsResult analyzeConfigDiagnostics(const autoinput::ConfigData& config,
                                                     const graph::GraphDocument* doc)
    {
        ConfigDiagnosticsResult result;

        // 1. Structure to track start input usage across commands and sequences
        struct StartInputUsage
        {
            std::vector<std::size_t> commandIndices;
            std::vector<std::size_t> sequenceIndices;
        };
        std::unordered_map<std::string, StartInputUsage> startKeyUsageMap;

        // 2. Track general command input triggers (keys / buttons) to detect shared inputs
        std::unordered_map<std::string, std::vector<std::size_t>> commandInputMap;

        // Analyze commands
        for (std::size_t cmdIdx = 0; cmdIdx < config.commands.size(); ++cmdIdx)
        {
            const auto& cmd = config.commands[cmdIdx];
            const std::string cmdName = getCommandDisplayName(cmd, cmdIdx);

            // Empty command name check
            if (cmd.name.empty())
            {
                result.issues.push_back(
                    ConfigDiagnosticIssue{ .severity = ConfigDiagnosticSeverity::Warning,
                                           .message = std::format("Command {} has an empty name.", cmdIdx + 1),
                                           .category = "Command",
                                           .commandIndex = cmdIdx,
                                           .suggestedFix = "Assign a descriptive name to the command." });
            }

            // Missing command action check
            if (cmd.action.empty())
            {
                result.issues.push_back(ConfigDiagnosticIssue{
                    .severity = ConfigDiagnosticSeverity::Error,
                    .message = std::format("Command '{}' has no action specified.", cmdName),
                    .category = "Command",
                    .commandIndex = cmdIdx,
                    .suggestedFix = "Specify an action (e.g. 'press', 'repeat', 'toggle', 'hold')." });
            }

            // Controls diagnostics
            for (std::size_t ctrlIdx = 0; ctrlIdx < cmd.controls.size(); ++ctrlIdx)
            {
                const auto& ctrl = cmd.controls[ctrlIdx];
                if (ctrl.input.empty())
                {
                    result.issues.push_back(ConfigDiagnosticIssue{
                        .severity = ConfigDiagnosticSeverity::Error,
                        .message =
                            std::format("Command '{}' control {} has an empty input binding.", cmdName, ctrlIdx + 1),
                        .category = "Control",
                        .commandIndex = cmdIdx,
                        .controlIndex = ctrlIdx,
                        .suggestedFix = "Specify a valid key or mouse button trigger for this control." });
                }

                if (ctrl.action.empty())
                {
                    result.issues.push_back(ConfigDiagnosticIssue{
                        .severity = ConfigDiagnosticSeverity::Warning,
                        .message =
                            std::format("Command '{}' control {} has no action specified.", cmdName, ctrlIdx + 1),
                        .category = "Control",
                        .commandIndex = cmdIdx,
                        .controlIndex = ctrlIdx,
                        .suggestedFix = "Specify a control action (e.g. 'pause', 'cancel', 'resume')." });
                }
            }

            // Start keys analysis
            std::unordered_set<std::string> localStartKeys;
            for (const auto& sk : cmd.startKeys)
            {
                const std::string norm = toLowerTrimmed(sk);
                if (norm.empty())
                {
                    continue;
                }

                if (localStartKeys.contains(norm))
                {
                    result.issues.push_back(ConfigDiagnosticIssue{
                        .severity = ConfigDiagnosticSeverity::Warning,
                        .message = std::format("Command '{}' defines duplicate start key '{}'.", cmdName, sk),
                        .category = "Command",
                        .commandIndex = cmdIdx,
                        .suggestedFix = "Remove redundant duplicate start keys from this command." });
                }
                localStartKeys.insert(norm);
                startKeyUsageMap[norm].commandIndices.push_back(cmdIdx);
            }

            // Input keys & buttons analysis
            for (const auto& k : cmd.keys)
            {
                const std::string norm = toLowerTrimmed(k);
                if (!norm.empty())
                {
                    auto& list = commandInputMap[norm];
                    if (std::find(list.begin(), list.end(), cmdIdx) == list.end())
                    {
                        list.push_back(cmdIdx);
                    }
                }
            }
            for (const auto& b : cmd.buttons)
            {
                const std::string norm = "btn:" + toLowerTrimmed(b);
                if (!norm.empty())
                {
                    auto& list = commandInputMap[norm];
                    if (std::find(list.begin(), list.end(), cmdIdx) == list.end())
                    {
                        list.push_back(cmdIdx);
                    }
                }
            }
        }

        // Analyze sequences
        for (std::size_t seqIdx = 0; seqIdx < config.sequences.size(); ++seqIdx)
        {
            const auto& seq = config.sequences[seqIdx];
            const std::string seqName = getSequenceDisplayName(seq, seqIdx);

            // Empty sequence name check
            if (seq.name.empty())
            {
                result.issues.push_back(
                    ConfigDiagnosticIssue{ .severity = ConfigDiagnosticSeverity::Warning,
                                           .message = std::format("Sequence {} has an empty name.", seqIdx + 1),
                                           .category = "Sequence",
                                           .sequenceIndex = seqIdx,
                                           .suggestedFix = "Assign a descriptive name to the sequence." });
            }

            // Empty sequence start check
            if (seq.start.empty())
            {
                result.issues.push_back(ConfigDiagnosticIssue{
                    .severity = ConfigDiagnosticSeverity::Warning,
                    .message = std::format("Sequence '{}' has no start trigger key configured.", seqName),
                    .category = "Sequence",
                    .sequenceIndex = seqIdx,
                    .suggestedFix = "Assign a hotkey to the sequence start property so it can be triggered." });
            }
            else
            {
                const std::string norm = toLowerTrimmed(seq.start);
                startKeyUsageMap[norm].sequenceIndices.push_back(seqIdx);
            }
        }

        // Check for duplicate start inputs across commands and sequences
        for (const auto& [inputKey, usage] : startKeyUsageMap)
        {
            const std::size_t totalUsage = usage.commandIndices.size() + usage.sequenceIndices.size();
            if (totalUsage > 1)
            {
                std::string sources;
                for (std::size_t ci : usage.commandIndices)
                {
                    if (!sources.empty()) sources += ", ";
                    sources += std::format("Command '{}'", getCommandDisplayName(config.commands[ci], ci));
                }
                for (std::size_t si : usage.sequenceIndices)
                {
                    if (!sources.empty()) sources += ", ";
                    sources += std::format("Sequence '{}'", getSequenceDisplayName(config.sequences[si], si));
                }

                result.issues.push_back(ConfigDiagnosticIssue{
                    .severity = ConfigDiagnosticSeverity::Warning,
                    .message = std::format("Duplicate start input '{}' is configured across multiple items: {}.",
                                           inputKey, sources),
                    .category = "Input Conflict",
                    .commandIndex = usage.commandIndices.empty()
                                        ? std::nullopt
                                        : std::optional<std::size_t>(usage.commandIndices.front()),
                    .sequenceIndex = usage.sequenceIndices.empty()
                                         ? std::nullopt
                                         : std::optional<std::size_t>(usage.sequenceIndices.front()),
                    .suggestedFix = "Assign unique trigger keys to prevent activation ambiguities." });
            }
        }

        // Check for commands sharing input keys or buttons
        for (const auto& [inputName, cmdIndices] : commandInputMap)
        {
            if (cmdIndices.size() > 1)
            {
                std::string cmdList;
                for (std::size_t ci : cmdIndices)
                {
                    if (!cmdList.empty()) cmdList += ", ";
                    cmdList += std::format("'{}'", getCommandDisplayName(config.commands[ci], ci));
                }

                const std::string cleanInput = inputName.starts_with("btn:") ? inputName.substr(4) : inputName;
                result.issues.push_back(
                    ConfigDiagnosticIssue{ .severity = ConfigDiagnosticSeverity::Info,
                                           .message = std::format("Commands {} share input '{}'.", cmdList, cleanInput),
                                           .category = "Input Conflict",
                                           .commandIndex = cmdIndices.front(),
                                           .suggestedFix = "Confirm that these commands are intended to share inputs "
                                                           "or isolate them using exclusive groups." });
            }
        }

        // Integrate core config validator findings
        const auto coreErrors = autoinput::validateConfigData(config);
        for (const auto& err : coreErrors)
        {
            ConfigDiagnosticSeverity sev = ConfigDiagnosticSeverity::Error;
            if (err.severity == autoinput::ValidationSeverity::Warning)
            {
                sev = ConfigDiagnosticSeverity::Warning;
            }
            else if (err.severity == autoinput::ValidationSeverity::Info)
            {
                sev = ConfigDiagnosticSeverity::Info;
            }

            result.issues.push_back(
                ConfigDiagnosticIssue{ .severity = sev,
                                       .message = err.message,
                                       .category = err.section.empty() ? "Configuration" : err.section,
                                       .suggestedFix = err.suggestedFix });
        }

        // Map issues to visual graph nodes if document is provided
        if (doc != nullptr)
        {
            mapIssuesToGraphNodes(result, *doc);
        }

        return result;
    }

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

                    ImVec4 color(0.7F, 0.7F, 0.7F, 1.0F);
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
