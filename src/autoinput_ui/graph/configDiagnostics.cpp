/**
 * @file configDiagnostics.cpp
 * @brief Implementation of reusable non-UI configuration diagnostics and validation helpers.
 * @author djsquiddy
 * @date August 2026
 */
#include "configDiagnostics.h"

#include "autoinput/config/configMetadata.h"
#include "autoinput/config/configValidator.h"
#include "autoinput/support/types.h"

#include <algorithm>
#include <cctype>
#include <format>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace autoinput::ui::graph
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
    } // namespace

    void mapDiagnosticsToGraphNodes(ConfigDiagnosticsResult& result, const GraphDocument& doc)
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
                    if (node.kind == NodeKind::Command && node.sourceIndex == issue.commandIndex)
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
                    if (node.kind == NodeKind::Sequence && node.sourceIndex == issue.sequenceIndex)
                    {
                        issue.associatedNodeId = node.id;
                        break;
                    }
                }
            }
        }
    }

    ConfigDiagnosticsResult analyzeConfigDiagnostics(const autoinput::ConfigData& config, const GraphDocument* doc)
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

        // 3. Track command and sequence names to detect duplicate names
        std::unordered_map<std::string, std::vector<std::size_t>> commandNameMap;
        std::unordered_map<std::string, std::vector<std::size_t>> sequenceNameMap;

        // Analyze commands
        for (std::size_t cmdIdx = 0; cmdIdx < config.commands.size(); ++cmdIdx)
        {
            const auto& cmd = config.commands[cmdIdx];
            const std::string cmdName = getCommandDisplayName(cmd, cmdIdx);
            const std::string normCmdName = toLowerTrimmed(cmd.name);

            // Empty command name check
            if (normCmdName.empty())
            {
                result.issues.push_back(
                    ConfigDiagnosticIssue{ .severity = ConfigDiagnosticSeverity::Warning,
                                           .message = std::format("Command {} has an empty name.", cmdIdx + 1),
                                           .category = "Command",
                                           .commandIndex = cmdIdx,
                                           .suggestedFix = "Assign a descriptive name to the command." });
            }
            else
            {
                commandNameMap[normCmdName].push_back(cmdIdx);
            }

            // Empty command action check
            if (toLowerTrimmed(cmd.action).empty())
            {
                result.issues.push_back(
                    ConfigDiagnosticIssue{ .severity = ConfigDiagnosticSeverity::Error,
                                           .message = std::format("Command '{}' has no action specified.", cmdName),
                                           .category = "Command",
                                           .commandIndex = cmdIdx,
                                           .suggestedFix = "Specify an action (e.g. 'click', 'hold')." });
            }
            else if (autoinput::actionStateFromArguments(cmd.action) == autoinput::ActionState::INVALID)
            {
                // Invalid command action check using existing metadata
                result.issues.push_back(ConfigDiagnosticIssue{
                    .severity = ConfigDiagnosticSeverity::Error,
                    .message = std::format("Command '{}' has invalid action '{}'. Valid choices: {}.", cmdName,
                                           cmd.action, autoinput::ConfigMetadata::validActionChoices()),
                    .category = "Command",
                    .commandIndex = cmdIdx,
                    .suggestedFix = "Use a valid action name such as 'click' or 'hold'." });
            }

            // Controls diagnostics
            for (std::size_t ctrlIdx = 0; ctrlIdx < cmd.controls.size(); ++ctrlIdx)
            {
                const auto& ctrl = cmd.controls[ctrlIdx];
                const bool emptyInput = toLowerTrimmed(ctrl.input).empty();
                const bool emptyAction = toLowerTrimmed(ctrl.action).empty();

                if (emptyInput && emptyAction)
                {
                    result.issues.push_back(ConfigDiagnosticIssue{
                        .severity = ConfigDiagnosticSeverity::Error,
                        .message = std::format("Command '{}' control {} is empty (no input binding or action).",
                                               cmdName, ctrlIdx + 1),
                        .category = "Control",
                        .commandIndex = cmdIdx,
                        .controlIndex = ctrlIdx,
                        .suggestedFix = "Specify a valid input trigger and control action." });
                }
                else if (emptyInput)
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
                else if (emptyAction)
                {
                    result.issues.push_back(ConfigDiagnosticIssue{
                        .severity = ConfigDiagnosticSeverity::Warning,
                        .message =
                            std::format("Command '{}' control {} has no action specified.", cmdName, ctrlIdx + 1),
                        .category = "Control",
                        .commandIndex = cmdIdx,
                        .controlIndex = ctrlIdx,
                        .relatedInput = ctrl.input,
                        .suggestedFix = "Specify a control action (e.g. 'pause', 'cancel', 'resume')." });
                }
                else if (autoinput::controlActionFromString(ctrl.action) == autoinput::ControlAction::Invalid)
                {
                    // Invalid control action check using metadata
                    result.issues.push_back(ConfigDiagnosticIssue{
                        .severity = ConfigDiagnosticSeverity::Error,
                        .message = std::format("Command '{}' control {} has invalid action '{}'. Valid choices: {}.",
                                               cmdName, ctrlIdx + 1, ctrl.action,
                                               autoinput::ConfigMetadata::validControlActionChoices()),
                        .category = "Control",
                        .commandIndex = cmdIdx,
                        .controlIndex = ctrlIdx,
                        .relatedInput = ctrl.input,
                        .suggestedFix = "Use a valid control action (start, toggle, stop, cancel, pause, resume, "
                                        "toggle-pause, stop-all, exit)." });
                }

                // Wildcard control diagnostic check
                if (!emptyInput && autoinput::ConfigMetadata::isWildcardTrigger(ctrl.input))
                {
                    result.issues.push_back(ConfigDiagnosticIssue{
                        .severity = ConfigDiagnosticSeverity::Info,
                        .message = std::format("Command '{}' control {} uses wildcard input trigger '{}' which "
                                               "intercepts multiple inputs.",
                                               cmdName, ctrlIdx + 1, ctrl.input),
                        .category = "Wildcard Control",
                        .commandIndex = cmdIdx,
                        .controlIndex = ctrlIdx,
                        .relatedInput = ctrl.input,
                        .suggestedFix =
                            "Ensure this wildcard control does not unintentionally block desired user input." });
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
                        .relatedInput = sk,
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

        // Detect duplicate command names
        for (const auto& [normName, indices] : commandNameMap)
        {
            if (indices.size() > 1)
            {
                for (std::size_t ci : indices)
                {
                    result.issues.push_back(ConfigDiagnosticIssue{
                        .severity = ConfigDiagnosticSeverity::Warning,
                        .message = std::format("Duplicate command name '{}' is used across {} commands.",
                                               config.commands[ci].name, indices.size()),
                        .category = "Command",
                        .commandIndex = ci,
                        .suggestedFix = "Assign unique names to each command." });
                }
            }
        }

        // Analyze sequences
        for (std::size_t seqIdx = 0; seqIdx < config.sequences.size(); ++seqIdx)
        {
            const auto& seq = config.sequences[seqIdx];
            const std::string seqName = getSequenceDisplayName(seq, seqIdx);
            const std::string normSeqName = toLowerTrimmed(seq.name);

            // Empty sequence name check
            if (normSeqName.empty())
            {
                result.issues.push_back(
                    ConfigDiagnosticIssue{ .severity = ConfigDiagnosticSeverity::Warning,
                                           .message = std::format("Sequence {} has an empty name.", seqIdx + 1),
                                           .category = "Sequence",
                                           .sequenceIndex = seqIdx,
                                           .suggestedFix = "Assign a descriptive name to the sequence." });
            }
            else
            {
                sequenceNameMap[normSeqName].push_back(seqIdx);
            }

            // Empty sequence start check
            if (toLowerTrimmed(seq.start).empty())
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

        // Detect duplicate sequence names
        for (const auto& [normName, indices] : sequenceNameMap)
        {
            if (indices.size() > 1)
            {
                for (std::size_t si : indices)
                {
                    result.issues.push_back(ConfigDiagnosticIssue{
                        .severity = ConfigDiagnosticSeverity::Warning,
                        .message = std::format("Duplicate sequence name '{}' is used across {} sequences.",
                                               config.sequences[si].name, indices.size()),
                        .category = "Sequence",
                        .sequenceIndex = si,
                        .suggestedFix = "Assign unique names to each sequence." });
                }
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

                std::string conflictMessage;
                if (!usage.commandIndices.empty() && !usage.sequenceIndices.empty())
                {
                    conflictMessage =
                        std::format("Command and sequence start conflict on input '{}': {}.", inputKey, sources);
                }
                else if (usage.commandIndices.size() > 1)
                {
                    conflictMessage =
                        std::format("Duplicate command start input '{}' is configured across multiple commands: {}.",
                                    inputKey, sources);
                }
                else
                {
                    conflictMessage =
                        std::format("Duplicate sequence start input '{}' is configured across multiple sequences: {}.",
                                    inputKey, sources);
                }

                result.issues.push_back(ConfigDiagnosticIssue{
                    .severity = ConfigDiagnosticSeverity::Warning,
                    .message = conflictMessage,
                    .category = "Input Conflict",
                    .commandIndex = usage.commandIndices.empty()
                                        ? std::nullopt
                                        : std::optional<std::size_t>(usage.commandIndices.front()),
                    .sequenceIndex = usage.sequenceIndices.empty()
                                         ? std::nullopt
                                         : std::optional<std::size_t>(usage.sequenceIndices.front()),
                    .relatedInput = inputKey,
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
                                           .relatedInput = cleanInput,
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

        // Map to graph document nodes if provided
        if (doc != nullptr)
        {
            mapDiagnosticsToGraphNodes(result, *doc);
        }

        return result;
    }
} // namespace autoinput::ui::graph
