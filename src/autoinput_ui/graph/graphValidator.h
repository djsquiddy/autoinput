/**
 * @file graphValidator.h
 * @brief Validation utilities for the dependency-free UI graph document model.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_GRAPH_GRAPH_VALIDATOR_H
#define INCLUDE_AUTOINPUT_UI_GRAPH_GRAPH_VALIDATOR_H

#include "graphModel.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace autoinput::ui::graph
{
    /**
     * @brief Severity levels for graph validation findings.
     */
    enum class ValidationSeverity : std::uint8_t
    {
        Info,
        Warning,
        Error
    };

    /**
     * @brief Converts a ValidationSeverity enum value to a human-readable string representation.
     */
    [[nodiscard]] constexpr std::string_view validationSeverityToString(ValidationSeverity severity) noexcept
    {
        switch (severity)
        {
        case ValidationSeverity::Info: return "Info";
        case ValidationSeverity::Warning: return "Warning";
        case ValidationSeverity::Error: return "Error";
        default: return "Unknown";
        }
    }

    /**
     * @brief Represents a single validation finding/issue in a graph document.
     */
    struct ValidationIssue
    {
        ValidationSeverity severity{ ValidationSeverity::Error };
        std::string message;
        std::optional<NodeId> nodeId{ std::nullopt };
        std::optional<LinkId> linkId{ std::nullopt };

        constexpr auto operator<=>(const ValidationIssue&) const = default;
    };

    using ValidationMessage = ValidationIssue;

    /**
     * @brief Configuration options for graph validation rules.
     */
    struct ValidationOptions
    {
        bool requireStartNode{ true };
        bool allowMultipleStartNodes{ false };
        bool requireEndNode{ true };
        bool allowDisconnectedNodes{ false };
        bool requireAcyclic{ true };
        bool allowSelfLinks{ false };
        bool treatDisconnectedAsError{ false };

        /**
         * @brief Default validation profile for execution sequence graphs (strict).
         */
        [[nodiscard]] static constexpr ValidationOptions sequenceGraph() noexcept
        {
            return ValidationOptions{ .requireStartNode = true,
                                      .allowMultipleStartNodes = false,
                                      .requireEndNode = true,
                                      .allowDisconnectedNodes = false,
                                      .requireAcyclic = true,
                                      .allowSelfLinks = false,
                                      .treatDisconnectedAsError = false };
        }

        /**
         * @brief Default validation profile for configuration viewer graphs (permissive/read-only).
         */
        [[nodiscard]] static constexpr ValidationOptions configGraph() noexcept
        {
            return ValidationOptions{ .requireStartNode = false,
                                      .allowMultipleStartNodes = true,
                                      .requireEndNode = false,
                                      .allowDisconnectedNodes = true,
                                      .requireAcyclic = false,
                                      .allowSelfLinks = true,
                                      .treatDisconnectedAsError = false };
        }
    };

    /**
     * @brief Summary result of a graph validation pass.
     */
    struct ValidationResult
    {
        std::vector<ValidationIssue> issues;

        [[nodiscard]] bool isValid() const noexcept { return !hasErrors(); }

        [[nodiscard]] bool hasErrors() const noexcept
        {
            return std::ranges::any_of(issues,
                                       [](const ValidationIssue& issue) noexcept
                                       { return issue.severity == ValidationSeverity::Error; });
        }

        [[nodiscard]] bool hasWarnings() const noexcept
        {
            return std::ranges::any_of(issues,
                                       [](const ValidationIssue& issue) noexcept
                                       { return issue.severity == ValidationSeverity::Warning; });
        }

        [[nodiscard]] std::size_t errorCount() const noexcept
        {
            return static_cast<std::size_t>(std::ranges::count_if(
                issues,
                [](const ValidationIssue& issue) noexcept { return issue.severity == ValidationSeverity::Error; }));
        }

        [[nodiscard]] std::size_t warningCount() const noexcept
        {
            return static_cast<std::size_t>(std::ranges::count_if(
                issues,
                [](const ValidationIssue& issue) noexcept { return issue.severity == ValidationSeverity::Warning; }));
        }

        [[nodiscard]] std::size_t infoCount() const noexcept
        {
            return static_cast<std::size_t>(std::ranges::count_if(
                issues,
                [](const ValidationIssue& issue) noexcept { return issue.severity == ValidationSeverity::Info; }));
        }

        constexpr auto operator<=>(const ValidationResult&) const = default;
    };

    /**
     * @brief Validates start node requirements (presence and uniqueness).
     */
    [[nodiscard]] std::vector<ValidationIssue> validateStartNodes(const GraphDocument& doc, bool requireStart = true,
                                                                  bool allowMultiple = false);

    /**
     * @brief Validates end node requirements (presence).
     */
    [[nodiscard]] std::vector<ValidationIssue> validateEndNodes(const GraphDocument& doc, bool requireEnd = true);

    /**
     * @brief Validates that all active nodes are connected to the graph topology.
     */
    [[nodiscard]] std::vector<ValidationIssue> validateDisconnectedNodes(
        const GraphDocument& doc, ValidationSeverity severity = ValidationSeverity::Warning);

    /**
     * @brief Validates that links reference existing pins.
     */
    [[nodiscard]] std::vector<ValidationIssue> validateLinkReferences(const GraphDocument& doc);

    /**
     * @brief Validates that links connect output pins to input pins and do not form illegal directions or self-links.
     */
    [[nodiscard]] std::vector<ValidationIssue> validateLinkDirections(const GraphDocument& doc,
                                                                      bool allowSelfLinks = false);

    /**
     * @brief Validates that the graph has no directed cycles (directed acyclic graph requirement).
     */
    [[nodiscard]] std::vector<ValidationIssue> validateAcyclic(const GraphDocument& doc);

    /**
     * @brief Executes comprehensive validation of a graph document against the specified options.
     */
    [[nodiscard]] ValidationResult validateGraph(const GraphDocument& doc,
                                                 const ValidationOptions& options = ValidationOptions::sequenceGraph());

} // namespace autoinput::ui::graph

#endif // INCLUDE_AUTOINPUT_UI_GRAPH_GRAPH_VALIDATOR_H
