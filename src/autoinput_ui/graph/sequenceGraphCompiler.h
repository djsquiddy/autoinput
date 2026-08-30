/**
 * @file sequenceGraphCompiler.h
 * @brief Compiler utilities to transform a sequence GraphDocument back into a RecordedSequence.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_GRAPH_SEQUENCE_GRAPH_COMPILER_H
#define INCLUDE_AUTOINPUT_UI_GRAPH_SEQUENCE_GRAPH_COMPILER_H

#include "graphModel.h"
#include "graphValidator.h"
#include "autoinput/config/config.h"
#include "autoinput/support/types.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace autoinput::ui::graph
{
    /**
     * @brief Compilation options and context for compiling a sequence graph back to a RecordedSequence.
     */
    struct SequenceCompileOptions
    {
        /**
         * @brief Default sequence name to use if not specified in sourceSequence or graph metadata.
         */
        std::string defaultName{ "Compiled Sequence" };

        /**
         * @brief Default trigger key to use if not specified in sourceSequence or graph metadata.
         */
        std::string defaultStart{};

        /**
         * @brief Default repeat setting if not specified in sourceSequence or graph metadata.
         */
        bool defaultRepeat{ false };

        /**
         * @brief Optional source sequence context to preserve non-graph fields and source event details.
         */
        std::optional<autoinput::RecordedSequence> sourceSequence{ std::nullopt };

        /**
         * @brief Returns default compile options.
         */
        [[nodiscard]] static SequenceCompileOptions defaults() { return SequenceCompileOptions{}; }
    };

    /**
     * @brief Structured result from compiling a sequence graph.
     */
    struct SequenceCompileResult
    {
        bool success{ false };
        std::optional<autoinput::RecordedSequence> sequence{ std::nullopt };
        std::vector<ValidationIssue> issues;

        [[nodiscard]] bool isSuccess() const noexcept { return success; }
        [[nodiscard]] explicit operator bool() const noexcept { return success; }

        [[nodiscard]] bool hasErrors() const noexcept
        {
            return std::ranges::any_of(issues, [](const ValidationIssue& issue) noexcept
                                       { return issue.severity == ValidationSeverity::Error; });
        }

        [[nodiscard]] bool hasWarnings() const noexcept
        {
            return std::ranges::any_of(issues, [](const ValidationIssue& issue) noexcept
                                       { return issue.severity == ValidationSeverity::Warning; });
        }

        [[nodiscard]] std::size_t errorCount() const noexcept
        {
            return static_cast<std::size_t>(
                std::ranges::count_if(issues, [](const ValidationIssue& issue) noexcept
                                      { return issue.severity == ValidationSeverity::Error; }));
        }

        [[nodiscard]] std::size_t warningCount() const noexcept
        {
            return static_cast<std::size_t>(
                std::ranges::count_if(issues, [](const ValidationIssue& issue) noexcept
                                      { return issue.severity == ValidationSeverity::Warning; }));
        }

        [[nodiscard]] bool operator==(const SequenceCompileResult& other) const noexcept
        {
            return success == other.success && sequence.has_value() == other.sequence.has_value() &&
                   issues == other.issues;
        }
    };

    /**
     * @brief Parses a RecordedEvent representation from a GraphNode's metadata (title and details).
     * @param node The graph node to inspect.
     * @return Reconstructed RecordedEvent.
     */
    [[nodiscard]] autoinput::RecordedEvent parseRecordedEventFromNode(const GraphNode& node);

    /**
     * @brief Compiles a GraphDocument back into a RecordedSequence following linear execution topology.
     *
     * Validates that the graph is a valid, single-entry, single-exit linear sequence without
     * cycles, unsupported branching, disconnected execution nodes, or illegal link directions.
     *
     * @param doc The graph document to compile.
     * @param options Compilation options and optional source sequence context.
     * @return SequenceCompileResult containing the compiled RecordedSequence if successful,
     *         or structured validation issues on failure.
     */
    [[nodiscard]] SequenceCompileResult compileGraphToSequence(
        const GraphDocument& doc, const SequenceCompileOptions& options = SequenceCompileOptions::defaults());

    /**
     * @brief Overload that compiles a GraphDocument using an existing RecordedSequence as context.
     *
     * @param doc The graph document to compile.
     * @param sourceSequence Source sequence to preserve name, trigger, repeat, and source event fidelity.
     * @return SequenceCompileResult containing the compiled RecordedSequence.
     */
    [[nodiscard]] inline SequenceCompileResult compileGraphToSequence(const GraphDocument& doc,
                                                                      const autoinput::RecordedSequence& sourceSequence)
    {
        SequenceCompileOptions options;
        options.sourceSequence = sourceSequence;
        options.defaultName = sourceSequence.name;
        options.defaultStart = sourceSequence.start;
        options.defaultRepeat = sourceSequence.repeat;
        return compileGraphToSequence(doc, options);
    }

} // namespace autoinput::ui::graph

#endif // INCLUDE_AUTOINPUT_UI_GRAPH_SEQUENCE_GRAPH_COMPILER_H
