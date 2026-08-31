/**
 * @file configGraphViewer.cpp
 * @brief Implementation of visual graph viewer, relationship editor, and diagnostics inspector for ConfigData.
 * @author djsquiddy
 * @date August 2026
 */
#include "configGraphViewer.h"

#include "../core/localization.h"
#include "../widgets/formWidgets.h"
#include "autoinput/config/configMetadata.h"
#include "autoinput/config/configValidator.h"

#include <algorithm>
#include <array>
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
        constexpr std::array<std::string_view, 9> controlActionNames = { "start",        "toggle",   "stop",
                                                                         "cancel",       "pause",    "resume",
                                                                         "toggle-pause", "stop-all", "exit" };
    } // namespace

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

        statusMessage = std::format(
            "Config graph ready ({} nodes, {} links).", graphDocument.nodeCount(), graphDocument.linkCount());
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

    bool ConfigGraphViewerState::beginCommandEdit(std::size_t commandIndex, const autoinput::ConfigData& config,
                                                  bool force)
    {
        if (commandIndex >= config.commands.size())
        {
            return false;
        }

        if (editDraft.isActive && editDraft.isDirty && editDraft.commandIndex == commandIndex && !force)
        {
            return true;
        }

        const auto& cmd = config.commands[commandIndex];
        editDraft.commandIndex = commandIndex;
        editDraft.name = cmd.name;
        editDraft.exclusiveGroup = cmd.exclusiveGroup;
        editDraft.startKeys = cmd.startKeys;
        editDraft.controls = cmd.controls;
        editDraft.isActive = true;
        editDraft.isDirty = false;
        editDraft.draftIssues.clear();
        validateDraft(config);
        return true;
    }

    void ConfigGraphViewerState::cancelCommandEdit()
    {
        editDraft = CommandEditDraft{};
    }

    void ConfigGraphViewerState::setCommandName(std::string_view name)
    {
        if (!editDraft.isActive)
        {
            return;
        }
        editDraft.name = name;
        editDraft.isDirty = true;
    }

    void ConfigGraphViewerState::setExclusiveGroup(std::string_view group)
    {
        if (!editDraft.isActive)
        {
            return;
        }
        editDraft.exclusiveGroup = group;
        editDraft.isDirty = true;
    }

    void ConfigGraphViewerState::addStartKey(std::string_view key)
    {
        if (!editDraft.isActive)
        {
            return;
        }
        editDraft.startKeys.push_back(std::string(key));
        editDraft.isDirty = true;
    }

    bool ConfigGraphViewerState::removeStartKey(std::size_t index)
    {
        if (!editDraft.isActive || index >= editDraft.startKeys.size())
        {
            return false;
        }
        editDraft.startKeys.erase(editDraft.startKeys.begin() + static_cast<std::ptrdiff_t>(index));
        editDraft.isDirty = true;
        return true;
    }

    bool ConfigGraphViewerState::setStartKey(std::size_t index, std::string_view key)
    {
        if (!editDraft.isActive || index >= editDraft.startKeys.size())
        {
            return false;
        }
        editDraft.startKeys[index] = key;
        editDraft.isDirty = true;
        return true;
    }

    void ConfigGraphViewerState::addControl(std::string_view action, std::string_view input)
    {
        if (!editDraft.isActive)
        {
            return;
        }
        editDraft.controls.push_back(
            autoinput::CommandControlData{ .action = std::string(action), .input = std::string(input) });
        editDraft.isDirty = true;
    }

    bool ConfigGraphViewerState::removeControl(std::size_t index)
    {
        if (!editDraft.isActive || index >= editDraft.controls.size())
        {
            return false;
        }
        editDraft.controls.erase(editDraft.controls.begin() + static_cast<std::ptrdiff_t>(index));
        editDraft.isDirty = true;
        return true;
    }

    bool ConfigGraphViewerState::updateControl(std::size_t index, std::string_view action, std::string_view input)
    {
        if (!editDraft.isActive || index >= editDraft.controls.size())
        {
            return false;
        }
        editDraft.controls[index].action = action;
        editDraft.controls[index].input = input;
        editDraft.isDirty = true;
        return true;
    }

    bool ConfigGraphViewerState::validateDraft(const autoinput::ConfigData& baseConfig)
    {
        editDraft.draftIssues.clear();
        if (!editDraft.isActive || editDraft.commandIndex >= baseConfig.commands.size())
        {
            return false;
        }

        // 1. Validate command name
        std::string trimmedName = editDraft.name;
        trimmedName.erase(0, trimmedName.find_first_not_of(" \t\n\r"));
        if (!trimmedName.empty())
        {
            trimmedName.erase(trimmedName.find_last_not_of(" \t\n\r") + 1);
        }
        if (trimmedName.empty())
        {
            editDraft.draftIssues.push_back(
                ConfigDiagnosticIssue{ .severity = ConfigDiagnosticSeverity::Error,
                                       .message = "Command name cannot be empty.",
                                       .category = "Command",
                                       .commandIndex = editDraft.commandIndex,
                                       .suggestedFix = "Provide a unique descriptive name for this command." });
        }

        // 2. Validate controls
        const auto validActions = autoinput::ConfigMetadata::validControlActionAliases();
        for (std::size_t i = 0; i < editDraft.controls.size(); ++i)
        {
            const auto& ctrl = editDraft.controls[i];
            std::string actionLower = ctrl.action;
            std::ranges::transform(
                actionLower, actionLower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            actionLower.erase(0, actionLower.find_first_not_of(" \t\n\r"));
            if (!actionLower.empty())
            {
                actionLower.erase(actionLower.find_last_not_of(" \t\n\r") + 1);
            }

            if (actionLower.empty())
            {
                editDraft.draftIssues.push_back(ConfigDiagnosticIssue{
                    .severity = ConfigDiagnosticSeverity::Error,
                    .message = std::format("Control #{} has empty action.", i + 1),
                    .category = "Control",
                    .commandIndex = editDraft.commandIndex,
                    .controlIndex = i,
                    .suggestedFix =
                        "Specify a valid control action (e.g. 'start', 'toggle', 'stop', 'pause', 'resume')." });
            }
            else
            {
                const bool isValidAction =
                    std::ranges::any_of(validActions, [&](std::string_view valid) { return valid == actionLower; });
                if (!isValidAction)
                {
                    editDraft.draftIssues.push_back(ConfigDiagnosticIssue{
                        .severity = ConfigDiagnosticSeverity::Error,
                        .message = std::format("Control #{} has unknown action '{}'.", i + 1, ctrl.action),
                        .category = "Control",
                        .commandIndex = editDraft.commandIndex,
                        .controlIndex = i,
                        .suggestedFix =
                            std::format("Use one of: {}", autoinput::ConfigMetadata::validControlActionChoices()) });
                }
            }

            if (ctrl.input.empty())
            {
                editDraft.draftIssues.push_back(ConfigDiagnosticIssue{
                    .severity = ConfigDiagnosticSeverity::Warning,
                    .message = std::format("Control #{} has empty trigger input.", i + 1),
                    .category = "Control",
                    .commandIndex = editDraft.commandIndex,
                    .controlIndex = i,
                    .suggestedFix = "Specify a key, mouse button, or wildcard trigger (e.g. 'mouse.all')." });
            }
            else if (autoinput::ConfigMetadata::isWildcardTrigger(ctrl.input))
            {
                editDraft.draftIssues.push_back(ConfigDiagnosticIssue{
                    .severity = ConfigDiagnosticSeverity::Info,
                    .message = std::format("Control #{} uses wildcard trigger '{}'.", i + 1, ctrl.input),
                    .category = "Wildcard Control",
                    .commandIndex = editDraft.commandIndex,
                    .controlIndex = i,
                    .relatedInput = ctrl.input });
            }
        }

        // 3. Provisional whole-config diagnostics
        autoinput::ConfigData provisional = baseConfig;
        provisional.commands[editDraft.commandIndex].name = editDraft.name;
        provisional.commands[editDraft.commandIndex].exclusiveGroup = editDraft.exclusiveGroup;
        provisional.commands[editDraft.commandIndex].startKeys = editDraft.startKeys;
        provisional.commands[editDraft.commandIndex].controls = editDraft.controls;

        const auto fullDiag = analyzeConfigDiagnostics(provisional);
        for (const auto& issue : fullDiag.issues)
        {
            if (issue.commandIndex == editDraft.commandIndex)
            {
                const bool alreadyExists = std::ranges::any_of(
                    editDraft.draftIssues,
                    [&](const auto& existing)
                    { return existing.message == issue.message && existing.category == issue.category; });
                if (!alreadyExists)
                {
                    editDraft.draftIssues.push_back(issue);
                }
            }
        }

        return !editDraft.hasErrors();
    }

    bool ConfigGraphViewerState::applyCommandEdit(autoinput::ConfigData& targetConfig, bool forceWithWarnings)
    {
        if (!validateDraft(targetConfig))
        {
            statusMessage = "Cannot apply changes: Staged draft contains validation errors.";
            return false;
        }

        if (editDraft.hasWarnings() && !forceWithWarnings)
        {
            showApplyWarningConfirmation = true;
            statusMessage = "Draft contains validation warnings. Confirmation required to apply.";
            return false;
        }

        targetConfig.commands[editDraft.commandIndex].name = editDraft.name;
        targetConfig.commands[editDraft.commandIndex].exclusiveGroup = editDraft.exclusiveGroup;
        targetConfig.commands[editDraft.commandIndex].startKeys = editDraft.startKeys;
        targetConfig.commands[editDraft.commandIndex].controls = editDraft.controls;
        editDraft.isDirty = false;
        showApplyWarningConfirmation = false;

        rebuildFromConfig(targetConfig);
        statusMessage =
            std::format("Applied changes to command '{}'.", targetConfig.commands[editDraft.commandIndex].name);
        return true;
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
                ImGui::TextColored(
                    ImVec4(0.95F, 0.3F, 0.3F, 1.0F), "[Errors: %zu]", state.diagnosticsResult.errorCount());
            }
            else if (state.diagnosticsResult.hasWarnings())
            {
                ImGui::TextColored(
                    ImVec4(0.95F, 0.75F, 0.2F, 1.0F), "[Warnings: %zu]", state.diagnosticsResult.warningCount());
            }
            else
            {
                ImGui::TextColored(ImVec4(0.3F, 0.85F, 0.4F, 1.0F), "[Clean]");
            }

            if (state.hasUnappliedChanges())
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0F, 0.75F, 0.2F, 1.0F), "* Unapplied Draft");
            }

            if (filterChanged)
            {
                state.rebuildFromConfig(config);
                stateChanged = true;
            }

            ImGui::Separator();
            return stateChanged;
        }

        bool renderConfigNodeInspectorPanel(autoinput::ConfigData* mutableConfig, const autoinput::ConfigData& config,
                                            ConfigGraphViewerState& state)
        {
            bool stateChanged = false;
            ImGui::TextUnformatted("Node Inspector & Relationship Editor");
            ImGui::Separator();

            if (state.viewerState.selectedNodeId != graph::InvalidNodeId)
            {
                const auto details = inspectConfigGraphNode(
                    config, state.graphDocument, state.viewerState.selectedNodeId, &state.diagnosticsResult);
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

                    // Interactive editing section for Command / Control relationships
                    const bool isCommandOrControl =
                        (details->kind == graph::NodeKind::Command ||
                         (details->kind == graph::NodeKind::Control && details->commandIndex.has_value()));
                    if (mutableConfig != nullptr && state.isEditingAllowed && isCommandOrControl &&
                        details->commandIndex.has_value())
                    {
                        const std::size_t cmdIdx = *details->commandIndex;
                        if (cmdIdx < config.commands.size())
                        {
                            if (!state.editDraft.isActive ||
                                (state.editDraft.commandIndex != cmdIdx && !state.editDraft.isDirty))
                            {
                                state.beginCommandEdit(cmdIdx, config);
                            }

                            if (state.editDraft.isActive && state.editDraft.commandIndex == cmdIdx)
                            {
                                ImGui::Spacing();
                                ImGui::Separator();
                                ImGui::TextColored(ImVec4(0.4F, 0.8F, 1.0F, 1.0F), "Edit Command Relationships");

                                // 1. Command Name
                                if (widgets::StringInput("Name", state.editDraft.name))
                                {
                                    state.editDraft.isDirty = true;
                                    state.validateDraft(config);
                                }

                                // 2. Exclusive Group
                                if (widgets::StringInput("Exclusive Group", state.editDraft.exclusiveGroup))
                                {
                                    state.editDraft.isDirty = true;
                                    state.validateDraft(config);
                                }

                                // 3. Start Keys
                                ImGui::Spacing();
                                ImGui::TextDisabled("Start Keys (%zu):", state.editDraft.startKeys.size());
                                for (std::size_t k = 0; k < state.editDraft.startKeys.size(); ++k)
                                {
                                    ImGui::PushID(static_cast<int>(k));
                                    ImGui::SetNextItemWidth(140.0F);
                                    if (widgets::StringInput("##sk", state.editDraft.startKeys[k]))
                                    {
                                        state.editDraft.isDirty = true;
                                        state.validateDraft(config);
                                    }
                                    ImGui::SameLine();
                                    if (ImGui::SmallButton("Remove"))
                                    {
                                        state.removeStartKey(k);
                                        state.validateDraft(config);
                                        ImGui::PopID();
                                        break;
                                    }
                                    ImGui::PopID();
                                }

                                if (ImGui::SmallButton("+ Add Start Key"))
                                {
                                    state.addStartKey("f1");
                                    state.validateDraft(config);
                                }

                                // 4. Controls
                                ImGui::Spacing();
                                ImGui::TextDisabled("Controls (%zu):", state.editDraft.controls.size());
                                for (std::size_t c = 0; c < state.editDraft.controls.size(); ++c)
                                {
                                    ImGui::PushID(static_cast<int>(100 + c));
                                    auto& ctrl = state.editDraft.controls[c];

                                    ImGui::SetNextItemWidth(100.0F);
                                    if (widgets::StringCombo("##ctrlAct", ctrl.action, controlActionNames))
                                    {
                                        state.editDraft.isDirty = true;
                                        state.validateDraft(config);
                                    }
                                    ImGui::SameLine();
                                    ImGui::SetNextItemWidth(110.0F);
                                    if (widgets::StringInput("##ctrlIn", ctrl.input))
                                    {
                                        state.editDraft.isDirty = true;
                                        state.validateDraft(config);
                                    }
                                    ImGui::SameLine();

                                    if (ImGui::SmallButton("Preset"))
                                    {
                                        ImGui::OpenPopup("CtrlPresetPopup");
                                    }

                                    if (ImGui::BeginPopup("CtrlPresetPopup"))
                                    {
                                        ImGui::TextDisabled("Wildcards");
                                        if (ImGui::MenuItem("mouse.all (Any Mouse)"))
                                        {
                                            ctrl.input = "mouse.all";
                                            state.editDraft.isDirty = true;
                                            state.validateDraft(config);
                                        }
                                        if (ImGui::MenuItem("keys.all (Any Key)"))
                                        {
                                            ctrl.input = "keys.all";
                                            state.editDraft.isDirty = true;
                                            state.validateDraft(config);
                                        }
                                        if (ImGui::MenuItem("input.all (Any Input)"))
                                        {
                                            ctrl.input = "input.all";
                                            state.editDraft.isDirty = true;
                                            state.validateDraft(config);
                                        }
                                        ImGui::Separator();
                                        ImGui::TextDisabled("Mouse Buttons");
                                        if (ImGui::MenuItem("mouse.left"))
                                        {
                                            ctrl.input = "mouse.left";
                                            state.editDraft.isDirty = true;
                                            state.validateDraft(config);
                                        }
                                        if (ImGui::MenuItem("mouse.right"))
                                        {
                                            ctrl.input = "mouse.right";
                                            state.editDraft.isDirty = true;
                                            state.validateDraft(config);
                                        }
                                        if (ImGui::MenuItem("mouse.middle"))
                                        {
                                            ctrl.input = "mouse.middle";
                                            state.editDraft.isDirty = true;
                                            state.validateDraft(config);
                                        }
                                        ImGui::EndPopup();
                                    }

                                    ImGui::SameLine();
                                    if (ImGui::SmallButton("X"))
                                    {
                                        state.removeControl(c);
                                        state.validateDraft(config);
                                        ImGui::PopID();
                                        break;
                                    }
                                    ImGui::PopID();
                                }

                                if (ImGui::SmallButton("+ Add Control"))
                                {
                                    state.addControl("start", "mouse.left");
                                    state.validateDraft(config);
                                }

                                // 5. Draft Diagnostics
                                if (state.editDraft.isDirty)
                                {
                                    ImGui::Spacing();
                                    if (state.editDraft.hasErrors())
                                    {
                                        ImGui::TextColored(ImVec4(0.95F, 0.3F, 0.3F, 1.0F),
                                                           "Draft Errors (%zu):",
                                                           state.editDraft.draftIssues.size());
                                        for (const auto& issue : state.editDraft.draftIssues)
                                        {
                                            if (issue.severity == ConfigDiagnosticSeverity::Error)
                                            {
                                                ImGui::BulletText("%s", issue.message.c_str());
                                            }
                                        }
                                    }
                                    else if (state.editDraft.hasWarnings())
                                    {
                                        ImGui::TextColored(ImVec4(0.95F, 0.75F, 0.2F, 1.0F),
                                                           "Draft Warnings (%zu):",
                                                           state.editDraft.draftIssues.size());
                                        for (const auto& issue : state.editDraft.draftIssues)
                                        {
                                            if (issue.severity == ConfigDiagnosticSeverity::Warning)
                                            {
                                                ImGui::BulletText("%s", issue.message.c_str());
                                            }
                                        }
                                    }
                                    else
                                    {
                                        ImGui::TextColored(ImVec4(0.3F, 0.85F, 0.4F, 1.0F), "[Draft Valid]");
                                    }

                                    ImGui::Spacing();
                                    const bool canApply = !state.editDraft.hasErrors();
                                    if (!canApply)
                                    {
                                        ImGui::BeginDisabled();
                                    }
                                    if (ImGui::Button("Apply to Config"))
                                    {
                                        if (state.applyCommandEdit(*mutableConfig))
                                        {
                                            stateChanged = true;
                                        }
                                    }
                                    if (!canApply)
                                    {
                                        ImGui::EndDisabled();
                                    }

                                    ImGui::SameLine();
                                    if (ImGui::Button("Revert Changes"))
                                    {
                                        state.beginCommandEdit(state.editDraft.commandIndex, config, true);
                                    }

                                    if (state.showApplyWarningConfirmation)
                                    {
                                        ImGui::Spacing();
                                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 0.85F, 0.2F, 1.0F));
                                        ImGui::Text("Notice: Draft contains validation warning(s). Apply anyway?");
                                        ImGui::PopStyleColor();
                                        ImGui::SameLine();
                                        if (ImGui::Button("Confirm Apply"))
                                        {
                                            if (state.applyCommandEdit(*mutableConfig, true))
                                            {
                                                stateChanged = true;
                                            }
                                        }
                                        ImGui::SameLine();
                                        if (ImGui::Button("Cancel"))
                                        {
                                            state.dismissApplyWarningConfirmation();
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (isCommandOrControl && mutableConfig == nullptr)
                    {
                        // Read-only inspection details for command/controls
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
                    }
                    else
                    {
                        // Read-only elements (Sequences, Filters, Groups, Inputs)
                        if (!details->connectedInputs.empty())
                        {
                            ImGui::Spacing();
                            ImGui::TextDisabled("Inputs:");
                            for (const auto& inText : details->connectedInputs)
                            {
                                ImGui::BulletText("%s", inText.c_str());
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

                        ImGui::Spacing();
                        ImGui::TextDisabled("Note: This element is read-only in the graph editor. Use the dedicated "
                                            "editor tabs to modify sequence events or global settings.");
                    }

                    if (!details->diagnosticIssues.empty())
                    {
                        ImGui::Spacing();
                        ImGui::TextColored(
                            ImVec4(0.95F, 0.4F, 0.3F, 1.0F), "Diagnostics (%zu):", details->diagnosticIssues.size());
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
                ImGui::TextDisabled("Select a node on the graph to inspect properties or edit relationships.");
            }
            ImGui::Spacing();
            ImGui::Separator();
            return stateChanged;
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

    bool renderConfigGraphViewer(autoinput::ConfigData& config, ConfigGraphViewerState& state, const char* viewerId)
    {
        state.syncWithConfig(config);

#if defined(AUTOINPUT_UI_INTERNAL_HAS_IMGUI)
        bool stateChanged = false;

        ImGui::PushID(viewerId);

        // Handle keyboard shortcuts when not typing text
        const ImGuiIO& io = ImGui::GetIO();
        if (!io.WantTextInput && !ImGui::IsAnyItemActive())
        {
            if (ImGui::IsKeyPressed(ImGuiKey_Escape) && state.editDraft.isActive)
            {
                state.cancelCommandEdit();
                stateChanged = true;
            }
            else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Enter) && state.editDraft.isActive &&
                     !state.editDraft.hasErrors())
            {
                if (state.applyCommandEdit(config))
                {
                    stateChanged = true;
                }
            }
        }

        if (state.showFilterToolbar)
        {
            if (renderConfigFilterToolbar(config, state))
            {
                stateChanged = true;
            }
        }

        const float inspectorWidth = (state.showInspectorPanel || state.showDiagnosticsPanel) ? 360.0F : 0.0F;
        const float canvasWidth =
            ImGui::GetContentRegionAvail().x - (inspectorWidth > 0.0F ? inspectorWidth + 10.0F : 0.0F);

        ImGui::BeginChild("ConfigGraphCanvasChild", ImVec2(canvasWidth, 0.0F), true);
        {
            graph::renderFallbackGraphViewer(
                state.graphDocument, state.viewerState, graph::ValidationOptions::configGraph(), "ConfigGraphCanvas");
        }
        ImGui::EndChild();

        if (inspectorWidth > 0.0F)
        {
            ImGui::SameLine();
            ImGui::BeginChild("ConfigGraphInspectorSidePanel", ImVec2(inspectorWidth, 0.0F), true);
            {
                if (state.showInspectorPanel)
                {
                    if (renderConfigNodeInspectorPanel(&config, config, state))
                    {
                        stateChanged = true;
                    }
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

        // Context menu for config graph
        if (ImGui::BeginPopupContextWindow("ConfigGraphContextMenu",
                                           ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            ImGui::TextDisabled("Filter Toggles");
            if (ImGui::MenuItem("Show Commands", nullptr, state.showCommands))
            {
                state.showCommands = !state.showCommands;
                state.rebuildFromConfig(config);
                stateChanged = true;
            }
            if (ImGui::MenuItem("Show Controls", nullptr, state.showControls))
            {
                state.showControls = !state.showControls;
                state.rebuildFromConfig(config);
                stateChanged = true;
            }
            if (ImGui::MenuItem("Show Sequences", nullptr, state.showSequences))
            {
                state.showSequences = !state.showSequences;
                state.rebuildFromConfig(config);
                stateChanged = true;
            }
            if (ImGui::MenuItem("Show Inputs", nullptr, state.showInputs))
            {
                state.showInputs = !state.showInputs;
                state.rebuildFromConfig(config);
                stateChanged = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Rebuild Graph"))
            {
                state.rebuildFromConfig(config);
                stateChanged = true;
            }
            if (state.editDraft.isActive && ImGui::MenuItem("Cancel Draft Edit (Esc)"))
            {
                state.cancelCommandEdit();
                stateChanged = true;
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
        return stateChanged;
#else
        return false;
#endif
    }

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

        const float inspectorWidth = (state.showInspectorPanel || state.showDiagnosticsPanel) ? 360.0F : 0.0F;
        const float canvasWidth =
            ImGui::GetContentRegionAvail().x - (inspectorWidth > 0.0F ? inspectorWidth + 10.0F : 0.0F);

        ImGui::BeginChild("ConfigGraphCanvasChild", ImVec2(canvasWidth, 0.0F), true);
        {
            graph::renderFallbackGraphViewer(
                state.graphDocument, state.viewerState, graph::ValidationOptions::configGraph(), "ConfigGraphCanvas");
        }
        ImGui::EndChild();

        if (inspectorWidth > 0.0F)
        {
            ImGui::SameLine();
            ImGui::BeginChild("ConfigGraphInspectorSidePanel", ImVec2(inspectorWidth, 0.0F), true);
            {
                if (state.showInspectorPanel)
                {
                    renderConfigNodeInspectorPanel(nullptr, config, state);
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
