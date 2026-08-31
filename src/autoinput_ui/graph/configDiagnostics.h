/**
 * @file configDiagnostics.h
 * @brief Reusable non-UI diagnostics and validation helpers for AutoInput ConfigData.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_GRAPH_CONFIG_DIAGNOSTICS_H
#define INCLUDE_AUTOINPUT_UI_GRAPH_CONFIG_DIAGNOSTICS_H
#pragma once

#include "autoinput/config/config.h"
#include "autoinput_ui/graph/graphModel.h"

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
     * @brief Severity levels for configuration diagnostic and validation findings.
     */
    enum class ConfigDiagnosticSeverity : std::uint8_t
    {
        Info,
        Warning,
        Error
    };

    /**
     * @brief Converts diagnostic severity enum to human-readable string.
     */
    [[nodiscard]] constexpr std::string_view configDiagnosticSeverityToString(
        ConfigDiagnosticSeverity severity) noexcept
    {
        switch (severity)
        {
        case ConfigDiagnosticSeverity::Info: return "Info";
        case ConfigDiagnosticSeverity::Warning: return "Warning";
        case ConfigDiagnosticSeverity::Error: return "Error";
        default: return "Unknown";
        }
    }

    /**
     * @brief A structured diagnostic finding produced by analyzing configuration data or graph relationships.
     */
    struct ConfigDiagnosticIssue
    {
        ConfigDiagnosticSeverity severity{ ConfigDiagnosticSeverity::Warning };
        std::string message;
        std::string
            category; ///< e.g. "Command", "Control", "Sequence", "Input Conflict", "Wildcard Control", "Configuration"
        std::optional<std::size_t> commandIndex{ std::nullopt };
        std::optional<std::size_t> controlIndex{ std::nullopt };
        std::optional<std::size_t> sequenceIndex{ std::nullopt };
        std::optional<std::string> relatedInput{ std::nullopt };
        std::optional<NodeId> associatedNodeId{ std::nullopt };
        std::string suggestedFix;
    };

    /**
     * @brief Aggregate result of configuration diagnostics analysis.
     */
    struct ConfigDiagnosticsResult
    {
        std::vector<ConfigDiagnosticIssue> issues;

        [[nodiscard]] bool hasErrors() const noexcept
        {
            return std::ranges::any_of(
                issues, [](const auto& issue) { return issue.severity == ConfigDiagnosticSeverity::Error; });
        }

        [[nodiscard]] bool hasWarnings() const noexcept
        {
            return std::ranges::any_of(
                issues, [](const auto& issue) { return issue.severity == ConfigDiagnosticSeverity::Warning; });
        }

        [[nodiscard]] bool hasInfo() const noexcept
        {
            return std::ranges::any_of(
                issues, [](const auto& issue) { return issue.severity == ConfigDiagnosticSeverity::Info; });
        }

        [[nodiscard]] std::size_t errorCount() const noexcept
        {
            return static_cast<std::size_t>(std::ranges::count_if(
                issues, [](const auto& issue) { return issue.severity == ConfigDiagnosticSeverity::Error; }));
        }

        [[nodiscard]] std::size_t warningCount() const noexcept
        {
            return static_cast<std::size_t>(std::ranges::count_if(
                issues, [](const auto& issue) { return issue.severity == ConfigDiagnosticSeverity::Warning; }));
        }

        [[nodiscard]] std::size_t infoCount() const noexcept
        {
            return static_cast<std::size_t>(std::ranges::count_if(
                issues, [](const auto& issue) { return issue.severity == ConfigDiagnosticSeverity::Info; }));
        }

        [[nodiscard]] std::size_t totalCount() const noexcept { return issues.size(); }
    };

    /**
     * @brief Performs comprehensive diagnostic analysis of ConfigData.
     * @param config The configuration to analyze.
     * @param doc Optional GraphDocument to map diagnostic findings to visual graph node IDs.
     * @return Aggregate diagnostics result containing structured issues.
     */
    [[nodiscard]] ConfigDiagnosticsResult analyzeConfigDiagnostics(const autoinput::ConfigData& config,
                                                                   const GraphDocument* doc = nullptr);

    /**
     * @brief Maps diagnostic issues in a result to corresponding nodes in a GraphDocument.
     * @param result Diagnostics result to update in-place.
     * @param doc Visual graph document to search for corresponding nodes.
     */
    void mapDiagnosticsToGraphNodes(ConfigDiagnosticsResult& result, const GraphDocument& doc);
} // namespace autoinput::ui::graph

#endif // INCLUDE_AUTOINPUT_UI_GRAPH_CONFIG_DIAGNOSTICS_H
