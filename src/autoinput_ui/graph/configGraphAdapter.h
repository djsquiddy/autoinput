/**
 * @file configGraphAdapter.h
 * @brief Converter utilities to transform ConfigData into a GraphDocument visual model.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_GRAPH_CONFIG_GRAPH_ADAPTER_H
#define INCLUDE_AUTOINPUT_UI_GRAPH_CONFIG_GRAPH_ADAPTER_H

#include "graphModel.h"
#include "autoinput/config/config.h"

#include <cstddef>
#include <string>
#include <string_view>

namespace autoinput::ui::graph
{
    /**
     * @brief Options controlling layout and relationship generation for config graph conversion.
     */
    struct ConfigGraphOptions
    {
        /**
         * @brief Starting X coordinate for layout placement.
         */
        float startX{ 50.0F };

        /**
         * @brief Starting Y coordinate for layout placement.
         */
        float startY{ 50.0F };

        /**
         * @brief Horizontal column spacing between graph layers (Inputs -> Core entities -> Controls/Groups).
         */
        float columnSpacing{ 320.0F };

        /**
         * @brief Vertical spacing step between adjacent nodes in the same section.
         */
        float rowSpacing{ 90.0F };

        /**
         * @brief Vertical spacing gap between separate command/sequence blocks.
         */
        float blockSpacing{ 40.0F };

        /**
         * @brief Whether to generate nodes for global settings (application filter, blacklist, global end key).
         */
        bool includeGlobalSettings{ true };

        /**
         * @brief Whether to link global setting nodes (app filter, blacklist, end key) to affected commands/sequences.
         */
        bool linkGlobalSettingsToTargets{ true };

        /**
         * @brief When true, multiple commands with the same exclusive group point to a single shared group node.
         */
        bool deduplicateExclusiveGroups{ true };

        /**
         * @brief Returns default config graph conversion options.
         */
        [[nodiscard]] static constexpr ConfigGraphOptions defaults() noexcept { return ConfigGraphOptions{}; }
    };

    /**
     * @brief Formats a human-readable title for a command node.
     */
    [[nodiscard]] std::string formatCommandTitle(const autoinput::CommandData& command, std::size_t index);

    /**
     * @brief Formats a human-readable subtitle/details string for a command node.
     */
    [[nodiscard]] std::string formatCommandSubtitle(const autoinput::CommandData& command);

    /**
     * @brief Formats a human-readable title for a command control node.
     */
    [[nodiscard]] std::string formatControlTitle(const autoinput::CommandControlData& control, std::size_t index);

    /**
     * @brief Formats a human-readable subtitle/details string for a command control node.
     */
    [[nodiscard]] std::string formatControlSubtitle(const autoinput::CommandControlData& control);

    /**
     * @brief Formats a human-readable title for a sequence node in config view.
     */
    [[nodiscard]] std::string formatConfigSequenceTitle(const autoinput::RecordedSequence& sequence, std::size_t index);

    /**
     * @brief Formats a human-readable subtitle/details string for a sequence node in config view.
     */
    [[nodiscard]] std::string formatConfigSequenceSubtitle(const autoinput::RecordedSequence& sequence);

    /**
     * @brief Converts a ConfigData model into a read-only GraphDocument representation.
     *
     * The generated graph represents:
     * - Global application filters, blacklist entries, and global end keys
     * - Commands with their actions, press/release timings, and source indices
     * - Command start keys, input keys, and mouse buttons
     * - Command controls with action and input bindings
     * - Exclusive groups with shared membership
     * - Recorded sequences with start triggers
     * - Topological links establishing input -> command, command -> control, command -> group,
     *   sequence start -> sequence, and global setting relationships.
     *
     * @param config The input configuration data.
     * @param options Layout and relationship generation options.
     * @return The populated GraphDocument.
     */
    [[nodiscard]] GraphDocument configToGraphDocument(
        const autoinput::ConfigData& config, const ConfigGraphOptions& options = ConfigGraphOptions::defaults());

    /**
     * @brief Alias for configToGraphDocument.
     */
    [[nodiscard]] inline GraphDocument convertConfigToGraph(
        const autoinput::ConfigData& config, const ConfigGraphOptions& options = ConfigGraphOptions::defaults())
    {
        return configToGraphDocument(config, options);
    }

} // namespace autoinput::ui::graph

#endif // INCLUDE_AUTOINPUT_UI_GRAPH_CONFIG_GRAPH_ADAPTER_H
