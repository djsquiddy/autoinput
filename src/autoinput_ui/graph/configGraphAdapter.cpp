/**
 * @file configGraphAdapter.cpp
 * @brief Implementation of ConfigData to GraphDocument conversion utilities.
 * @author djsquiddy
 * @date August 2026
 */
#include "configGraphAdapter.h"

#include <algorithm>
#include <format>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace autoinput::ui::graph
{
    namespace
    {
        void appendGlobalSettingsNodes(GraphDocument& doc, const autoinput::ConfigData& config,
                                       const ConfigGraphOptions& options, float& currentY,
                                       std::vector<PinId>& globalOutputPins)
        {
            if (!options.includeGlobalSettings)
            {
                return;
            }

            const float col0X = options.startX;
            bool addedGlobalNode = false;

            if (!config.application.empty())
            {
                auto& appNode = doc.createNode(NodeKind::ApplicationFilter, "Application Filter", { col0X, currentY });
                appNode.subtitle = config.application;
                auto* filterPin = doc.createPin(appNode.id, PinDirection::Output, "Filter");
                if (filterPin != nullptr)
                {
                    globalOutputPins.push_back(filterPin->id);
                }
                currentY += options.rowSpacing;
                addedGlobalNode = true;
            }

            for (const auto& blacklistEntry : config.blacklist)
            {
                auto& blNode = doc.createNode(NodeKind::BlacklistEntry, "Blacklist Entry", { col0X, currentY });
                blNode.subtitle = blacklistEntry;
                auto* blockedPin = doc.createPin(blNode.id, PinDirection::Output, "Blocked");
                if (blockedPin != nullptr)
                {
                    globalOutputPins.push_back(blockedPin->id);
                }
                currentY += options.rowSpacing;
                addedGlobalNode = true;
            }

            if (!config.endKey.empty())
            {
                auto& endNode = doc.createNode(NodeKind::Input, "Global End Key", { col0X, currentY });
                endNode.subtitle = config.endKey;
                auto* endPin = doc.createPin(endNode.id, PinDirection::Output, "End");
                if (endPin != nullptr)
                {
                    globalOutputPins.push_back(endPin->id);
                }
                currentY += options.rowSpacing;
                addedGlobalNode = true;
            }

            if (addedGlobalNode)
            {
                currentY += options.blockSpacing;
            }
        }

        std::size_t appendCommandInputNodes(GraphDocument& doc, const autoinput::CommandData& command,
                                            std::size_t cmdIdx, PinId cmdInPinId, const ConfigGraphOptions& options,
                                            float blockStartY)
        {
            const float col0X = options.startX;
            float inputY = blockStartY;
            std::size_t inputCount = 0;

            for (const auto& startKey : command.startKeys)
            {
                auto& skNode = doc.createNode(NodeKind::Input, "Start Key", { col0X, inputY });
                skNode.subtitle = startKey;
                skNode.sourceIndex = cmdIdx;
                auto* outPin = doc.createPin(skNode.id, PinDirection::Output, "Trigger");
                if (outPin != nullptr && cmdInPinId != InvalidPinId)
                {
                    doc.createLink(outPin->id, cmdInPinId);
                }
                inputY += options.rowSpacing;
                ++inputCount;
            }

            for (const auto& key : command.keys)
            {
                auto& kNode = doc.createNode(NodeKind::Input, "Input Key", { col0X, inputY });
                kNode.subtitle = key;
                kNode.sourceIndex = cmdIdx;
                auto* outPin = doc.createPin(kNode.id, PinDirection::Output, "Input");
                if (outPin != nullptr && cmdInPinId != InvalidPinId)
                {
                    doc.createLink(outPin->id, cmdInPinId);
                }
                inputY += options.rowSpacing;
                ++inputCount;
            }

            for (const auto& button : command.buttons)
            {
                auto& btnNode = doc.createNode(NodeKind::Input, "Input Button", { col0X, inputY });
                btnNode.subtitle = button;
                btnNode.sourceIndex = cmdIdx;
                auto* outPin = doc.createPin(btnNode.id, PinDirection::Output, "Input");
                if (outPin != nullptr && cmdInPinId != InvalidPinId)
                {
                    doc.createLink(outPin->id, cmdInPinId);
                }
                inputY += options.rowSpacing;
                ++inputCount;
            }

            return inputCount;
        }

        std::size_t appendCommandControlNodes(GraphDocument& doc, const autoinput::CommandData& command,
                                              PinId cmdCtrlPinId, const ConfigGraphOptions& options, float blockStartY,
                                              float& controlEndY)
        {
            const float col2X = options.startX + 2.0F * options.columnSpacing;
            float controlY = blockStartY;
            std::size_t controlCount = 0;

            for (std::size_t ctrlIdx = 0; ctrlIdx < command.controls.size(); ++ctrlIdx)
            {
                const auto& control = command.controls[ctrlIdx];
                auto& ctrlNode =
                    doc.createNode(NodeKind::Control, formatControlTitle(control, ctrlIdx), { col2X, controlY });
                ctrlNode.subtitle = formatControlSubtitle(control);
                ctrlNode.sourceIndex = ctrlIdx;

                auto* cInPin = doc.createPin(ctrlNode.id, PinDirection::Input, "Command");
                if (cInPin != nullptr && cmdCtrlPinId != InvalidPinId)
                {
                    doc.createLink(cmdCtrlPinId, cInPin->id);
                }
                controlY += options.rowSpacing;
                ++controlCount;
            }

            controlEndY = controlY;
            return controlCount;
        }

        std::size_t appendExclusiveGroupNode(
            GraphDocument& doc, const autoinput::CommandData& command, PinId cmdGrpPinId,
            const ConfigGraphOptions& options, float blockStartY, float controlEndY,
            std::unordered_map<std::string, std::pair<NodeId, PinId>>& sharedExclusiveGroups)
        {
            if (command.exclusiveGroup.empty())
            {
                return 0;
            }

            const float col2X = options.startX + 2.0F * options.columnSpacing;
            PinId groupMemberPinId = InvalidPinId;

            if (options.deduplicateExclusiveGroups && sharedExclusiveGroups.contains(command.exclusiveGroup))
            {
                groupMemberPinId = sharedExclusiveGroups[command.exclusiveGroup].second;
            }
            else
            {
                const float egY = std::max(controlEndY, blockStartY);
                auto& egNode = doc.createNode(NodeKind::ExclusiveGroup, "Exclusive Group", { col2X, egY });
                egNode.subtitle = command.exclusiveGroup;
                auto* mPin = doc.createPin(egNode.id, PinDirection::Input, "Member");
                groupMemberPinId = mPin != nullptr ? mPin->id : InvalidPinId;

                if (options.deduplicateExclusiveGroups)
                {
                    sharedExclusiveGroups[command.exclusiveGroup] = { egNode.id, groupMemberPinId };
                }
            }

            if (groupMemberPinId != InvalidPinId && cmdGrpPinId != InvalidPinId)
            {
                doc.createLink(cmdGrpPinId, groupMemberPinId);
            }

            return 1;
        }

        void appendCommandNodes(GraphDocument& doc, const autoinput::ConfigData& config,
                                const ConfigGraphOptions& options, float& currentY,
                                const std::vector<PinId>& globalOutputPins)
        {
            const float col1X = options.startX + options.columnSpacing;
            std::unordered_map<std::string, std::pair<NodeId, PinId>> sharedExclusiveGroups;

            for (std::size_t cmdIdx = 0; cmdIdx < config.commands.size(); ++cmdIdx)
            {
                const auto& command = config.commands[cmdIdx];
                const float blockStartY = currentY;

                // Command Node
                auto& cmdNode =
                    doc.createNode(NodeKind::Command, formatCommandTitle(command, cmdIdx), { col1X, blockStartY });
                cmdNode.subtitle = formatCommandSubtitle(command);
                cmdNode.sourceIndex = cmdIdx;

                auto* inPin = doc.createPin(cmdNode.id, PinDirection::Input, "Input");
                const PinId cmdInPinId = inPin != nullptr ? inPin->id : InvalidPinId;

                auto* ctrlPin = doc.createPin(cmdNode.id, PinDirection::Output, "Control");
                const PinId cmdCtrlPinId = ctrlPin != nullptr ? ctrlPin->id : InvalidPinId;

                auto* grpPin = doc.createPin(cmdNode.id, PinDirection::Output, "Group");
                const PinId cmdGrpPinId = grpPin != nullptr ? grpPin->id : InvalidPinId;

                const std::size_t inputCount =
                    appendCommandInputNodes(doc, command, cmdIdx, cmdInPinId, options, blockStartY);

                float controlEndY = blockStartY;
                const std::size_t controlCount =
                    appendCommandControlNodes(doc, command, cmdCtrlPinId, options, blockStartY, controlEndY);

                const std::size_t groupCount = appendExclusiveGroupNode(doc, command, cmdGrpPinId, options, blockStartY,
                                                                        controlEndY, sharedExclusiveGroups);

                if (options.linkGlobalSettingsToTargets && cmdInPinId != InvalidPinId)
                {
                    for (const auto gPinId : globalOutputPins)
                    {
                        doc.createLink(gPinId, cmdInPinId);
                    }
                }

                const std::size_t maxRows =
                    std::max({ inputCount, controlCount + groupCount, static_cast<std::size_t>(1) });
                currentY = blockStartY + static_cast<float>(maxRows) * options.rowSpacing + options.blockSpacing;
            }
        }

        void appendSequenceNodes(GraphDocument& doc, const autoinput::ConfigData& config,
                                 const ConfigGraphOptions& options, float& currentY,
                                 const std::vector<PinId>& globalOutputPins)
        {
            const float col0X = options.startX;
            const float col1X = options.startX + options.columnSpacing;

            for (std::size_t seqIdx = 0; seqIdx < config.sequences.size(); ++seqIdx)
            {
                const auto& sequence = config.sequences[seqIdx];
                const float blockStartY = currentY;

                auto& seqNode = doc.createNode(NodeKind::Sequence, formatConfigSequenceTitle(sequence, seqIdx),
                                               { col1X, blockStartY });
                seqNode.subtitle = formatConfigSequenceSubtitle(sequence);
                seqNode.sourceIndex = seqIdx;

                auto* sInPin = doc.createPin(seqNode.id, PinDirection::Input, "Trigger");
                const PinId seqInPinId = sInPin != nullptr ? sInPin->id : InvalidPinId;

                if (!sequence.start.empty())
                {
                    auto& skNode = doc.createNode(NodeKind::Input, "Sequence Start Key", { col0X, blockStartY });
                    skNode.subtitle = sequence.start;
                    skNode.sourceIndex = seqIdx;
                    auto* outPin = doc.createPin(skNode.id, PinDirection::Output, "Trigger");
                    if (outPin != nullptr && seqInPinId != InvalidPinId)
                    {
                        doc.createLink(outPin->id, seqInPinId);
                    }
                }

                if (options.linkGlobalSettingsToTargets && seqInPinId != InvalidPinId)
                {
                    for (const auto gPinId : globalOutputPins)
                    {
                        doc.createLink(gPinId, seqInPinId);
                    }
                }

                currentY = blockStartY + options.rowSpacing + options.blockSpacing;
            }
        }
    } // namespace

    std::string formatCommandTitle(const autoinput::CommandData& command, std::size_t index)
    {
        if (!command.name.empty())
        {
            return command.name;
        }
        return std::format("Command {}", index + 1);
    }

    std::string formatCommandSubtitle(const autoinput::CommandData& command)
    {
        std::string result = std::format("Action: {}", command.action.empty() ? "None" : command.action);

        if (!command.pressWait.empty() || !command.releaseWait.empty())
        {
            result += std::format(" | Press: {} | Release: {}", command.pressWait.empty() ? "0ms" : command.pressWait,
                                  command.releaseWait.empty() ? "0ms" : command.releaseWait);
        }

        if (!command.exclusiveGroup.empty())
        {
            result += std::format(" | Group: {}", command.exclusiveGroup);
        }

        return result;
    }

    std::string formatControlTitle(const autoinput::CommandControlData& control, std::size_t index)
    {
        if (!control.action.empty())
        {
            return std::format("Control: {}", control.action);
        }
        return std::format("Control {}", index + 1);
    }

    std::string formatControlSubtitle(const autoinput::CommandControlData& control)
    {
        return std::format("Action: {} | Input: {}", control.action.empty() ? "None" : control.action,
                           control.input.empty() ? "None" : control.input);
    }

    std::string formatConfigSequenceTitle(const autoinput::RecordedSequence& sequence, std::size_t index)
    {
        if (!sequence.name.empty())
        {
            return sequence.name;
        }
        return std::format("Sequence {}", index + 1);
    }

    std::string formatConfigSequenceSubtitle(const autoinput::RecordedSequence& sequence)
    {
        std::string result =
            std::format("Events: {} | Repeat: {}", sequence.events.size(), sequence.repeat ? "Yes" : "No");
        if (!sequence.start.empty())
        {
            result += std::format(" | Start: {}", sequence.start);
        }
        return result;
    }

    GraphDocument configToGraphDocument(const autoinput::ConfigData& config, const ConfigGraphOptions& options)
    {
        GraphDocument doc;
        float currentY = options.startY;
        std::vector<PinId> globalOutputPins;

        appendGlobalSettingsNodes(doc, config, options, currentY, globalOutputPins);
        appendCommandNodes(doc, config, options, currentY, globalOutputPins);
        appendSequenceNodes(doc, config, options, currentY, globalOutputPins);

        return doc;
    }

} // namespace autoinput::ui::graph
