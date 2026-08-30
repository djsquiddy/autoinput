/**
 * @file sequenceGraphAdapter.h
 * @brief Converter and adapter utilities to transform RecordedSequence into a GraphDocument.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_GRAPH_SEQUENCE_GRAPH_ADAPTER_H
#define INCLUDE_AUTOINPUT_UI_GRAPH_SEQUENCE_GRAPH_ADAPTER_H

#include "graphModel.h"
#include "autoinput/config/config.h"
#include "autoinput/support/types.h"

#include <cstddef>
#include <string>
#include <string_view>

namespace autoinput::ui::graph
{
    /**
     * @brief Options controlling layout and node decomposition for sequence graph conversion.
     */
    struct SequenceGraphOptions
    {
        /**
         * @brief When true, events with non-zero delays generate separate Wait nodes in the graph.
         * When false, delay information is retained in the event node's subtitle/metadata.
         */
        bool separateWaitNodes{ false };

        /**
         * @brief Initial X coordinate for the Start node.
         */
        float startX{ 50.0F };

        /**
         * @brief Initial Y coordinate for the Start node.
         */
        float startY{ 100.0F };

        /**
         * @brief Horizontal spacing step between sequential nodes.
         */
        float stepX{ 200.0F };

        /**
         * @brief Vertical spacing step between sequential nodes.
         */
        float stepY{ 0.0F };

        /**
         * @brief Returns default sequence graph layout and conversion options.
         */
        [[nodiscard]] static constexpr SequenceGraphOptions defaults() noexcept { return SequenceGraphOptions{}; }
    };

    /**
     * @brief Checks whether a delay string represents a non-zero time interval.
     * @param delay Delay string (e.g. "120ms", "1s", "0ms").
     * @return True if delay is valid and greater than zero.
     */
    [[nodiscard]] bool isNonZeroDelay(std::string_view delay) noexcept;

    /**
     * @brief Formats a human-readable title for a recorded event node.
     * @param event The recorded event data.
     * @return Readable title string (e.g. "Key Down", "Mouse Move").
     */
    [[nodiscard]] std::string formatRecordedEventTitle(const autoinput::RecordedEvent& event);

    /**
     * @brief Formats a human-readable subtitle/details string for a recorded event node.
     * @param event The recorded event data.
     * @param includeDelay Whether to include delay information in the subtitle.
     * @return Readable subtitle string.
     */
    [[nodiscard]] std::string formatRecordedEventSubtitle(const autoinput::RecordedEvent& event,
                                                          bool includeDelay = true);

    /**
     * @brief Converts a RecordedSequence into a linear GraphDocument representation.
     *
     * The generated graph consists of:
     * - Exactly one Start node with an output pin
     * - One RecordedEvent node per sequence event (preserving source index)
     * - Optional Wait nodes if separateWaitNodes is true and non-zero delay exists
     * - Exactly one End node with an input pin
     * - Directed links connecting all nodes sequentially in execution order
     *
     * @param sequence The input recorded sequence.
     * @param options Layout and decomposition options.
     * @return The populated GraphDocument.
     */
    [[nodiscard]] GraphDocument sequenceToGraphDocument(
        const autoinput::RecordedSequence& sequence,
        const SequenceGraphOptions& options = SequenceGraphOptions::defaults());

    /**
     * @brief Alias for sequenceToGraphDocument.
     */
    [[nodiscard]] inline GraphDocument convertSequenceToGraph(
        const autoinput::RecordedSequence& sequence,
        const SequenceGraphOptions& options = SequenceGraphOptions::defaults())
    {
        return sequenceToGraphDocument(sequence, options);
    }

} // namespace autoinput::ui::graph

#endif // INCLUDE_AUTOINPUT_UI_GRAPH_SEQUENCE_GRAPH_ADAPTER_H
