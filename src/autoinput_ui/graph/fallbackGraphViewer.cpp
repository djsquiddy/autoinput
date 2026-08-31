/**
 * @file fallbackGraphViewer.cpp
 * @brief Implementation of simple dependency-free ImGui fallback graph viewer.
 * @author djsquiddy
 * @date August 2026
 */
#include "fallbackGraphViewer.h"

#include <algorithm>
#include <cctype>
#include <format>
#include <limits>
#include <ranges>

#if __has_include(<imgui.h>)
#include <imgui.h>
#include <imgui_stdlib.h>
#define AUTOINPUT_UI_INTERNAL_HAS_IMGUI 1
#endif

namespace autoinput::ui::graph
{
    namespace
    {
        [[nodiscard]] std::string toLowerString(std::string_view sv)
        {
            std::string result;
            result.reserve(sv.size());
            for (char ch : sv)
            {
                result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
            }
            return result;
        }

        // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
        [[nodiscard]] bool containsIgnoreCase(std::string_view text, std::string_view query)
        {
            if (query.empty())
            {
                return true;
            }
            auto lowerText = toLowerString(text);
            auto lowerQuery = toLowerString(query);
            return lowerText.contains(lowerQuery);
        }
    } // namespace

    std::string formatNodeHeader(const GraphNode& node)
    {
        std::string kindStr = std::string(nodeKindToString(node.kind));
        if (node.title.empty())
        {
            return std::format("[{}] Node #{}", kindStr, node.id);
        }
        return std::format("[{}] {}", kindStr, node.title);
    }

    std::string formatPinSummary(const GraphPin& pin)
    {
        std::string dirStr = (pin.direction == PinDirection::Input) ? "Input" : "Output";
        if (pin.name.empty())
        {
            return std::format("Pin #{} ({})", pin.id, dirStr);
        }
        return std::format("{} ({})", pin.name, dirStr);
    }

    std::string formatLinkSummary(const GraphDocument& doc, const GraphLink& link)
    {
        const auto* startPin = doc.findPin(link.fromPinId);
        const auto* endPin = doc.findPin(link.toPinId);
        const auto* startNode = startPin ? doc.findNode(startPin->nodeId) : nullptr;
        const auto* endNode = endPin ? doc.findNode(endPin->nodeId) : nullptr;

        std::string startNodeName = "Unknown Node";
        if (startNode != nullptr)
        {
            startNodeName = startNode->title.empty() ? std::format("Node #{}", startNode->id) : startNode->title;
        }

        std::string startPinName = "Unknown Pin";
        if (startPin != nullptr)
        {
            startPinName = startPin->name.empty() ? std::format("Pin #{}", startPin->id) : startPin->name;
        }

        std::string endNodeName = "Unknown Node";
        if (endNode != nullptr)
        {
            endNodeName = endNode->title.empty() ? std::format("Node #{}", endNode->id) : endNode->title;
        }

        std::string endPinName = "Unknown Pin";
        if (endPin != nullptr)
        {
            endPinName = endPin->name.empty() ? std::format("Pin #{}", endPin->id) : endPin->name;
        }

        return std::format("Link #{}: {}:{} -> {}:{}", link.id, startNodeName, startPinName, endNodeName, endPinName);
    }

    std::vector<ValidationIssue> getNodeValidationIssues(const ValidationResult& result, NodeId nodeId)
    {
        std::vector<ValidationIssue> issues;
        for (const auto& issue : result.issues)
        {
            if (issue.nodeId.has_value() && *issue.nodeId == nodeId)
            {
                issues.push_back(issue);
            }
        }
        return issues;
    }

    std::vector<ValidationIssue> getLinkValidationIssues(const ValidationResult& result, LinkId linkId)
    {
        std::vector<ValidationIssue> issues;
        for (const auto& issue : result.issues)
        {
            if (issue.linkId.has_value() && *issue.linkId == linkId)
            {
                issues.push_back(issue);
            }
        }
        return issues;
    }

    std::vector<LinkId> getNodeIncomingLinks(const GraphDocument& doc, NodeId nodeId)
    {
        const auto* node = doc.findNode(nodeId);
        if (!node)
        {
            return {};
        }

        std::vector<LinkId> incoming;
        for (PinId pinId : node->pinIds)
        {
            const auto* pin = doc.findPin(pinId);
            if (pin && pin->direction == PinDirection::Input)
            {
                for (const auto& link : doc.links())
                {
                    if (link.toPinId == pin->id)
                    {
                        incoming.push_back(link.id);
                    }
                }
            }
        }
        return incoming;
    }

    std::vector<LinkId> getNodeOutgoingLinks(const GraphDocument& doc, NodeId nodeId)
    {
        const auto* node = doc.findNode(nodeId);
        if (!node)
        {
            return {};
        }

        std::vector<LinkId> outgoing;
        for (PinId pinId : node->pinIds)
        {
            const auto* pin = doc.findPin(pinId);
            if (pin && pin->direction == PinDirection::Output)
            {
                for (const auto& link : doc.links())
                {
                    if (link.fromPinId == pin->id)
                    {
                        outgoing.push_back(link.id);
                    }
                }
            }
        }
        return outgoing;
    }

    std::pair<NodePosition, NodePosition> computeGraphBoundingBox(const GraphDocument& doc)
    {
        if (doc.nodes().empty())
        {
            return { NodePosition{ .x = 0.0F, .y = 0.0F }, NodePosition{ .x = 0.0F, .y = 0.0F } };
        }

        float minX = std::numeric_limits<float>::max();
        float minY = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float maxY = std::numeric_limits<float>::lowest();

        for (const auto& node : doc.nodes())
        {
            minX = std::min(minX, node.position.x);
            minY = std::min(minY, node.position.y);
            maxX = std::max(maxX, node.position.x);
            maxY = std::max(maxY, node.position.y);
        }

        return { NodePosition{ .x = minX, .y = minY }, NodePosition{ .x = maxX, .y = maxY } };
    }

    std::vector<NodeId> filterGraphNodes(const GraphDocument& doc, std::string_view searchFilter,
                                         std::optional<NodeKind> kindFilter, const ValidationResult* validationResult,
                                         bool onlyProblematic)
    {
        std::vector<NodeId> matches;
        for (const auto& node : doc.nodes())
        {
            if (kindFilter.has_value() && node.kind != *kindFilter)
            {
                continue;
            }

            if (!searchFilter.empty())
            {
                bool matchTitle = containsIgnoreCase(node.title, searchFilter);
                bool matchSubtitle = containsIgnoreCase(node.subtitle, searchFilter);
                bool matchKind = containsIgnoreCase(nodeKindToString(node.kind), searchFilter);
                bool matchId = containsIgnoreCase(std::to_string(node.id), searchFilter);

                if (!matchTitle && !matchSubtitle && !matchKind && !matchId)
                {
                    continue;
                }
            }

            if (onlyProblematic && validationResult)
            {
                auto issues = getNodeValidationIssues(*validationResult, node.id);
                if (issues.empty())
                {
                    continue;
                }
            }

            matches.push_back(node.id);
        }
        return matches;
    }

#ifdef AUTOINPUT_UI_INTERNAL_HAS_IMGUI

    namespace
    {
        [[nodiscard]] ImVec4 getSeverityColor(ValidationSeverity severity) noexcept
        {
            switch (severity)
            {
            case ValidationSeverity::Info: return ImVec4(0.4F, 0.7F, 1.0F, 1.0F);     // Cyan-Blue
            case ValidationSeverity::Warning: return ImVec4(1.0F, 0.85F, 0.2F, 1.0F); // Yellow
            case ValidationSeverity::Error: return ImVec4(1.0F, 0.35F, 0.35F, 1.0F);  // Soft Red
            default: return ImVec4(1.0F, 1.0F, 1.0F, 1.0F);
            }
        }

        [[nodiscard]] ImU32 getNodeKindHeaderColor(NodeKind kind) noexcept
        {
            switch (kind)
            {
            case NodeKind::Start: return IM_COL32(46, 125, 50, 255);            // Green
            case NodeKind::End: return IM_COL32(198, 40, 40, 255);              // Red
            case NodeKind::RecordedEvent: return IM_COL32(21, 101, 192, 255);   // Blue
            case NodeKind::Wait: return IM_COL32(239, 108, 0, 255);             // Orange
            case NodeKind::Command: return IM_COL32(0, 131, 143, 255);          // Cyan
            case NodeKind::Control: return IM_COL32(106, 27, 154, 255);         // Purple
            case NodeKind::Input: return IM_COL32(0, 105, 92, 255);             // Teal
            case NodeKind::Sequence: return IM_COL32(40, 53, 147, 255);         // Indigo
            case NodeKind::ExclusiveGroup: return IM_COL32(216, 67, 21, 255);   // Deep Orange
            case NodeKind::ApplicationFilter: return IM_COL32(55, 71, 79, 255); // Blue Grey
            case NodeKind::BlacklistEntry: return IM_COL32(78, 52, 46, 255);    // Brown
            case NodeKind::Comment: return IM_COL32(97, 97, 97, 255);           // Grey
            case NodeKind::Unknown:
            default: return IM_COL32(66, 66, 66, 255);
            }
        }

        void renderInspectorSection(const GraphDocument& doc, const ValidationResult& validationResult,
                                    FallbackGraphViewerState& state)
        {
            ImGui::TextDisabled("INSPECTOR");
            ImGui::Separator();

            if (state.selectedNodeId != InvalidNodeId)
            {
                const auto* node = doc.findNode(state.selectedNodeId);
                if (node)
                {
                    ImGui::Text("Node ID: %llu", static_cast<unsigned long long>(node->id));
                    ImGui::Text("Kind: %s", std::string(nodeKindToString(node->kind)).c_str());
                    ImGui::Text("Title: %s", node->title.c_str());

                    if (!node->subtitle.empty())
                    {
                        ImGui::TextWrapped("Details: %s", node->subtitle.c_str());
                    }

                    if (node->sourceIndex.has_value())
                    {
                        ImGui::Text("Source Event Index: %zu", *node->sourceIndex);
                    }

                    ImGui::Text("Position: (%.1f, %.1f)", node->position.x, node->position.y);

                    ImGui::Spacing();
                    ImGui::TextDisabled("Pins (%zu total):", node->pinIds.size());

                    for (PinId pinId : node->pinIds)
                    {
                        const auto* pin = doc.findPin(pinId);
                        if (pin)
                        {
                            ImGui::BulletText("[%s] Pin #%llu: %s",
                                              pin->direction == PinDirection::Input ? "In" : "Out",
                                              static_cast<unsigned long long>(pin->id),
                                              pin->name.empty() ? "(unnamed)" : pin->name.c_str());
                        }
                    }

                    auto incomingLinks = getNodeIncomingLinks(doc, node->id);
                    auto outgoingLinks = getNodeOutgoingLinks(doc, node->id);

                    ImGui::Spacing();
                    ImGui::TextDisabled("Connections:");
                    ImGui::Text("Incoming Links: %zu", incomingLinks.size());
                    for (auto lid : incomingLinks)
                    {
                        const auto* lk = doc.findLink(lid);
                        if (lk)
                        {
                            ImGui::BulletText("<- Link #%llu", static_cast<unsigned long long>(lk->id));
                        }
                    }

                    ImGui::Text("Outgoing Links: %zu", outgoingLinks.size());
                    for (auto lid : outgoingLinks)
                    {
                        const auto* lk = doc.findLink(lid);
                        if (lk)
                        {
                            ImGui::BulletText("-> Link #%llu", static_cast<unsigned long long>(lk->id));
                        }
                    }

                    auto nodeIssues = getNodeValidationIssues(validationResult, node->id);
                    if (!nodeIssues.empty())
                    {
                        ImGui::Spacing();
                        ImGui::TextDisabled("Node Validation Issues (%zu):", nodeIssues.size());
                        for (const auto& issue : nodeIssues)
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text, getSeverityColor(issue.severity));
                            ImGui::BulletText("[%s] %s",
                                              std::string(validationSeverityToString(issue.severity)).c_str(),
                                              issue.message.c_str());
                            ImGui::PopStyleColor();
                        }
                    }
                }
                else
                {
                    ImGui::TextDisabled("Selected node #%llu no longer exists.",
                                        static_cast<unsigned long long>(state.selectedNodeId));
                }
            }
            else if (state.selectedLinkId != InvalidLinkId)
            {
                const auto* link = doc.findLink(state.selectedLinkId);
                if (link)
                {
                    ImGui::Text("Link ID: %llu", static_cast<unsigned long long>(link->id));
                    ImGui::Text("Start Pin ID: %llu", static_cast<unsigned long long>(link->fromPinId));
                    ImGui::Text("End Pin ID: %llu", static_cast<unsigned long long>(link->toPinId));
                    ImGui::TextWrapped("%s", formatLinkSummary(doc, *link).c_str());

                    auto linkIssues = getLinkValidationIssues(validationResult, link->id);
                    if (!linkIssues.empty())
                    {
                        ImGui::Spacing();
                        ImGui::TextDisabled("Link Validation Issues (%zu):", linkIssues.size());
                        for (const auto& issue : linkIssues)
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text, getSeverityColor(issue.severity));
                            ImGui::BulletText("[%s] %s",
                                              std::string(validationSeverityToString(issue.severity)).c_str(),
                                              issue.message.c_str());
                            ImGui::PopStyleColor();
                        }
                    }
                }
                else
                {
                    ImGui::TextDisabled("Selected link #%llu no longer exists.",
                                        static_cast<unsigned long long>(state.selectedLinkId));
                }
            }
            else
            {
                ImGui::TextDisabled("Select a node or link from the list or canvas to view detailed properties.");
            }
        }

        void renderNodesTable(const GraphDocument& doc, const ValidationResult& validationResult,
                              FallbackGraphViewerState& state)
        {
            auto filteredNodeIds = filterGraphNodes(
                doc, state.searchFilter, state.filterKind, &validationResult, state.showOnlyProblematicNodes);

            if (ImGui::BeginTable("FallbackNodesTable",
                                  5,
                                  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                                      ImGuiTableFlags_ScrollY))
            {
                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 45.0F);
                ImGui::TableSetupColumn("Kind", ImGuiTableColumnFlags_WidthFixed, 100.0F);
                ImGui::TableSetupColumn("Title", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Pins", ImGuiTableColumnFlags_WidthFixed, 60.0F);
                ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 70.0F);
                ImGui::TableHeadersRow();

                for (NodeId nid : filteredNodeIds)
                {
                    const auto* node = doc.findNode(nid);
                    if (!node)
                    {
                        continue;
                    }

                    ImGui::TableNextRow();
                    bool isSelected = state.isNodeSelected(node->id);

                    // ID Column
                    ImGui::TableSetColumnIndex(0);
                    std::string selectIdStr = std::format("##NodeRow_{}", node->id);
                    if (ImGui::Selectable(selectIdStr.c_str(),
                                          isSelected,
                                          ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap))
                    {
                        state.selectNode(node->id);
                    }
                    ImGui::SameLine();
                    ImGui::Text("%llu", static_cast<unsigned long long>(node->id));

                    // Kind Column
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", std::string(nodeKindToString(node->kind)).c_str());

                    // Title Column
                    ImGui::TableSetColumnIndex(2);
                    if (!node->title.empty())
                    {
                        ImGui::Text("%s", node->title.c_str());
                    }
                    else
                    {
                        ImGui::TextDisabled("(no title)");
                    }
                    if (!node->subtitle.empty())
                    {
                        ImGui::TextDisabled(" %s", node->subtitle.c_str());
                    }

                    // Pins Column
                    ImGui::TableSetColumnIndex(3);
                    std::size_t inPins = 0;
                    std::size_t outPins = 0;
                    for (PinId pinId : node->pinIds)
                    {
                        const auto* pin = doc.findPin(pinId);
                        if (pin)
                        {
                            if (pin->direction == PinDirection::Input)
                                ++inPins;
                            else
                                ++outPins;
                        }
                    }
                    ImGui::Text("%zu / %zu", inPins, outPins);

                    // Status Column
                    ImGui::TableSetColumnIndex(4);
                    auto nodeIssues = getNodeValidationIssues(validationResult, node->id);
                    if (nodeIssues.empty())
                    {
                        ImGui::TextColored(ImVec4(0.4F, 0.9F, 0.4F, 1.0F), "OK");
                    }
                    else
                    {
                        bool hasErr = false;
                        for (const auto& is : nodeIssues)
                        {
                            if (is.severity == ValidationSeverity::Error)
                            {
                                hasErr = true;
                                break;
                            }
                        }
                        if (hasErr)
                        {
                            ImGui::TextColored(ImVec4(1.0F, 0.35F, 0.35F, 1.0F), "[!] Error");
                        }
                        else
                        {
                            ImGui::TextColored(ImVec4(1.0F, 0.85F, 0.2F, 1.0F), "[!] Warn");
                        }
                    }
                }
                ImGui::EndTable();
            }
        }

        void renderLinksTable(const GraphDocument& doc, const ValidationResult& validationResult,
                              FallbackGraphViewerState& state)
        {
            if (ImGui::BeginTable("FallbackLinksTable",
                                  4,
                                  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                                      ImGuiTableFlags_ScrollY))
            {
                ImGui::TableSetupColumn("Link ID", ImGuiTableColumnFlags_WidthFixed, 60.0F);
                ImGui::TableSetupColumn("Source -> Destination", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Pins (From -> To)", ImGuiTableColumnFlags_WidthFixed, 130.0F);
                ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 70.0F);
                ImGui::TableHeadersRow();

                for (const auto& link : doc.links())
                {
                    ImGui::TableNextRow();
                    bool isSelected = state.isLinkSelected(link.id);

                    ImGui::TableSetColumnIndex(0);
                    std::string linkSelectId = std::format("##LinkRow_{}", link.id);
                    if (ImGui::Selectable(linkSelectId.c_str(),
                                          isSelected,
                                          ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap))
                    {
                        state.selectLink(link.id);
                    }
                    ImGui::SameLine();
                    ImGui::Text("%llu", static_cast<unsigned long long>(link.id));

                    ImGui::TableSetColumnIndex(1);
                    const auto* startPin = doc.findPin(link.fromPinId);
                    const auto* endPin = doc.findPin(link.toPinId);
                    const auto* startNode = startPin ? doc.findNode(startPin->nodeId) : nullptr;
                    const auto* endNode = endPin ? doc.findNode(endPin->nodeId) : nullptr;

                    std::string srcText = startNode ? (startNode->title.empty() ? std::format("Node #{}", startNode->id)
                                                                                : startNode->title)
                                                    : "Unknown";
                    std::string dstText =
                        endNode ? (endNode->title.empty() ? std::format("Node #{}", endNode->id) : endNode->title)
                                : "Unknown";

                    ImGui::Text("%s -> %s", srcText.c_str(), dstText.c_str());

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("Pin #%llu -> Pin #%llu",
                                static_cast<unsigned long long>(link.fromPinId),
                                static_cast<unsigned long long>(link.toPinId));

                    ImGui::TableSetColumnIndex(3);
                    auto linkIssues = getLinkValidationIssues(validationResult, link.id);
                    if (linkIssues.empty())
                    {
                        ImGui::TextColored(ImVec4(0.4F, 0.9F, 0.4F, 1.0F), "Valid");
                    }
                    else
                    {
                        ImGui::TextColored(ImVec4(1.0F, 0.35F, 0.35F, 1.0F), "[!] Invalid");
                    }
                }
                ImGui::EndTable();
            }
        }

        void renderCanvasView(const GraphDocument& doc, const ValidationResult& validationResult,
                              FallbackGraphViewerState& state)
        {
            ImVec2 canvasSize = ImGui::GetContentRegionAvail();
            if (canvasSize.x < 100.0F) canvasSize.x = 100.0F;
            if (canvasSize.y < 100.0F) canvasSize.y = 100.0F;

            ImVec2 canvasP0 = ImGui::GetCursorScreenPos();
            ImVec2 canvasP1 = ImVec2(canvasP0.x + canvasSize.x, canvasP0.y + canvasSize.y);

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->AddRectFilled(canvasP0, canvasP1, IM_COL32(30, 30, 35, 255));
            drawList->AddRect(canvasP0, canvasP1, IM_COL32(60, 60, 70, 255));

            // Grid background
            const float GRID_STEP = 32.0F * state.canvasZoom;
            for (float x = std::fmod(state.canvasOffset.x, GRID_STEP); x < canvasSize.x; x += GRID_STEP)
            {
                drawList->AddLine(
                    ImVec2(canvasP0.x + x, canvasP0.y), ImVec2(canvasP0.x + x, canvasP1.y), IM_COL32(45, 45, 52, 255));
            }
            for (float y = std::fmod(state.canvasOffset.y, GRID_STEP); y < canvasSize.y; y += GRID_STEP)
            {
                drawList->AddLine(
                    ImVec2(canvasP0.x, canvasP0.y + y), ImVec2(canvasP1.x, canvasP0.y + y), IM_COL32(45, 45, 52, 255));
            }

            // Draw links first so nodes render on top
            for (const auto& link : doc.links())
            {
                const auto* startPin = doc.findPin(link.fromPinId);
                const auto* endPin = doc.findPin(link.toPinId);
                const auto* startNode = startPin ? doc.findNode(startPin->nodeId) : nullptr;
                const auto* endNode = endPin ? doc.findNode(endPin->nodeId) : nullptr;

                if (startNode && endNode)
                {
                    ImVec2 pStart(
                        canvasP0.x + (startNode->position.x + 160.0F) * state.canvasZoom + state.canvasOffset.x,
                        canvasP0.y + (startNode->position.y + 40.0F) * state.canvasZoom + state.canvasOffset.y);
                    ImVec2 pEnd(canvasP0.x + (endNode->position.x) * state.canvasZoom + state.canvasOffset.x,
                                canvasP0.y + (endNode->position.y + 40.0F) * state.canvasZoom + state.canvasOffset.y);

                    ImU32 linkColor =
                        state.isLinkSelected(link.id) ? IM_COL32(255, 215, 0, 255) : IM_COL32(180, 180, 200, 220);
                    float linkThickness = state.isLinkSelected(link.id) ? 3.0F : 2.0F;

                    ImVec2 cp1(pStart.x + 50.0F * state.canvasZoom, pStart.y);
                    ImVec2 cp2(pEnd.x - 50.0F * state.canvasZoom, pEnd.y);
                    drawList->AddBezierCubic(pStart, cp1, cp2, pEnd, linkColor, linkThickness);
                }
            }

            // Draw nodes
            const float NODE_WIDTH = 160.0F;
            const float NODE_HEIGHT = 70.0F;

            ImVec2 mousePos = ImGui::GetIO().MousePos;
            bool mouseClicked = ImGui::IsMouseClicked(0);

            for (const auto& node : doc.nodes())
            {
                ImVec2 nodeMin(canvasP0.x + node.position.x * state.canvasZoom + state.canvasOffset.x,
                               canvasP0.y + node.position.y * state.canvasZoom + state.canvasOffset.y);
                ImVec2 nodeMax(nodeMin.x + NODE_WIDTH * state.canvasZoom, nodeMin.y + NODE_HEIGHT * state.canvasZoom);

                // Check click
                if (mouseClicked && mousePos.x >= nodeMin.x && mousePos.x <= nodeMax.x && mousePos.y >= nodeMin.y &&
                    mousePos.y <= nodeMax.y)
                {
                    state.selectNode(node.id);
                }

                bool isSelected = state.isNodeSelected(node.id);
                ImU32 headerColor = getNodeKindHeaderColor(node.kind);
                ImU32 bodyColor = IM_COL32(40, 44, 52, 240);
                ImU32 borderColor = isSelected ? IM_COL32(255, 215, 0, 255) : IM_COL32(90, 95, 105, 255);

                // Body & Header background
                drawList->AddRectFilled(nodeMin, nodeMax, bodyColor, 4.0F);
                ImVec2 headerMax(nodeMax.x, nodeMin.y + 22.0F * state.canvasZoom);
                drawList->AddRectFilled(nodeMin, headerMax, headerColor, 4.0F, ImDrawFlags_RoundCornersTop);
                drawList->AddRect(nodeMin, nodeMax, borderColor, 4.0F, 0, isSelected ? 2.5F : 1.0F);

                // Header text
                std::string headerText = node.title.empty() ? std::format("Node #{}", node.id) : node.title;
                if (headerText.size() > 18)
                {
                    headerText = headerText.substr(0, 16) + "...";
                }
                drawList->AddText(
                    ImVec2(nodeMin.x + 6.0F, nodeMin.y + 4.0F), IM_COL32(255, 255, 255, 255), headerText.c_str());

                // Subtitle text / Kind
                std::string kindStr = std::string(nodeKindToString(node.kind));
                drawList->AddText(
                    ImVec2(nodeMin.x + 6.0F, nodeMin.y + 26.0F), IM_COL32(180, 180, 190, 255), kindStr.c_str());

                if (!node.subtitle.empty())
                {
                    std::string sub = node.subtitle;
                    if (sub.size() > 20) sub = sub.substr(0, 18) + "..";
                    drawList->AddText(
                        ImVec2(nodeMin.x + 6.0F, nodeMin.y + 42.0F), IM_COL32(140, 140, 150, 255), sub.c_str());
                }

                // Pins indicators
                for (PinId pinId : node.pinIds)
                {
                    const auto* pin = doc.findPin(pinId);
                    if (pin)
                    {
                        if (pin->direction == PinDirection::Input)
                        {
                            ImVec2 pinPos(nodeMin.x, nodeMin.y + 40.0F * state.canvasZoom);
                            drawList->AddCircleFilled(pinPos, 4.0F, IM_COL32(0, 188, 212, 255));
                        }
                        else
                        {
                            ImVec2 pinPos(nodeMax.x, nodeMin.y + 40.0F * state.canvasZoom);
                            drawList->AddCircleFilled(pinPos, 4.0F, IM_COL32(255, 152, 0, 255));
                        }
                    }
                }
            }

            ImGui::Dummy(canvasSize);
        }

        void renderValidationMessagesList(const GraphDocument& /*doc*/, const ValidationResult& validationResult,
                                          FallbackGraphViewerState& state)
        {
            if (validationResult.issues.empty())
            {
                ImGui::TextColored(ImVec4(0.4F, 0.9F, 0.4F, 1.0F), "No validation issues found. Graph is valid.");
                return;
            }

            ImGui::Text("Validation Findings (%zu total):", validationResult.issues.size());
            ImGui::Separator();

            if (ImGui::BeginChild("ValidationIssuesScroll", ImVec2(0, 0), true))
            {
                for (size_t i = 0; i < validationResult.issues.size(); ++i)
                {
                    const auto& issue = validationResult.issues[i];
                    ImVec4 col = getSeverityColor(issue.severity);
                    std::string sevStr = std::string(validationSeverityToString(issue.severity));

                    ImGui::PushStyleColor(ImGuiCol_Text, col);

                    std::string issueLabel = std::format("[{}] {}", sevStr, issue.message);
                    if (issue.nodeId.has_value())
                    {
                        issueLabel += std::format(" (Node #{})", *issue.nodeId);
                    }
                    if (issue.linkId.has_value())
                    {
                        issueLabel += std::format(" (Link #{})", *issue.linkId);
                    }

                    if (ImGui::Selectable(issueLabel.c_str(), false))
                    {
                        if (issue.nodeId.has_value())
                        {
                            state.selectNode(*issue.nodeId);
                        }
                        else if (issue.linkId.has_value())
                        {
                            state.selectLink(*issue.linkId);
                        }
                    }
                    ImGui::PopStyleColor();
                }
            }
            ImGui::EndChild();
        }
    } // namespace

    void renderFallbackGraphViewer(const GraphDocument& doc, const ValidationResult& validationResult,
                                   FallbackGraphViewerState& state, const char* viewerId)
    {
        ImGui::PushID(viewerId);

        // Header Toolbar
        ImGui::BeginGroup();
        {
            ImGui::Text("Graph Document (%zu nodes, %zu links)", doc.nodes().size(), doc.links().size());
            ImGui::SameLine();

            if (validationResult.isValid())
            {
                ImGui::TextColored(ImVec4(0.4F, 0.9F, 0.4F, 1.0F), "[Valid]");
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0F, 0.35F, 0.35F, 1.0F),
                                   "[Validation: %zu error(s), %zu warning(s)]",
                                   validationResult.errorCount(),
                                   validationResult.warningCount());
            }

            ImGui::SameLine(ImGui::GetWindowWidth() - 260.0F);
            if (ImGui::Button("Reset View"))
            {
                state.clearSelection();
                state.canvasOffset = { 0.0F, 0.0F };
                state.canvasZoom = 1.0F;
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear Filter"))
            {
                state.searchFilter.clear();
                state.filterKind = std::nullopt;
                state.showOnlyProblematicNodes = false;
            }
        }
        ImGui::EndGroup();

        ImGui::Spacing();

        // Search & Filter Bar
        ImGui::SetNextItemWidth(180.0F);
        ImGui::InputTextWithHint("##SearchFilter", "Filter nodes...", &state.searchFilter);

        ImGui::SameLine();
        const char* viewModes[] = { "Split View", "List Only", "Canvas View" };
        int currentViewIndex = static_cast<int>(state.viewMode);
        ImGui::SetNextItemWidth(120.0F);
        if (ImGui::Combo("##ViewModeCombo", &currentViewIndex, viewModes, 3))
        {
            state.viewMode = static_cast<FallbackGraphViewMode>(currentViewIndex);
        }

        ImGui::SameLine();
        ImGui::Checkbox("Only Issues", &state.showOnlyProblematicNodes);

        ImGui::Separator();

        // Main Layout Rendering
        if (state.viewMode == FallbackGraphViewMode::Split)
        {
            float totalWidth = ImGui::GetContentRegionAvail().x;
            float leftPaneWidth = std::max(280.0F, totalWidth * 0.55F);

            if (ImGui::BeginChild("FallbackLeftPane", ImVec2(leftPaneWidth, 0), true))
            {
                if (ImGui::BeginTabBar("LeftPaneTabs"))
                {
                    if (ImGui::BeginTabItem("Nodes"))
                    {
                        renderNodesTable(doc, validationResult, state);
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Links"))
                    {
                        renderLinksTable(doc, validationResult, state);
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Canvas"))
                    {
                        renderCanvasView(doc, validationResult, state);
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Validation"))
                    {
                        renderValidationMessagesList(doc, validationResult, state);
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }
            }
            ImGui::EndChild();

            ImGui::SameLine();

            if (ImGui::BeginChild("FallbackRightPane", ImVec2(0, 0), true))
            {
                renderInspectorSection(doc, validationResult, state);
            }
            ImGui::EndChild();
        }
        else if (state.viewMode == FallbackGraphViewMode::ListOnly)
        {
            if (ImGui::BeginTabBar("ListOnlyTabs"))
            {
                if (ImGui::BeginTabItem("Nodes"))
                {
                    renderNodesTable(doc, validationResult, state);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Links"))
                {
                    renderLinksTable(doc, validationResult, state);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Inspector"))
                {
                    renderInspectorSection(doc, validationResult, state);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Validation"))
                {
                    renderValidationMessagesList(doc, validationResult, state);
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        else // Canvas mode
        {
            float totalWidth = ImGui::GetContentRegionAvail().x;
            float canvasWidth = std::max(300.0F, totalWidth * 0.65F);

            if (ImGui::BeginChild("FallbackCanvasPane", ImVec2(canvasWidth, 0), true))
            {
                renderCanvasView(doc, validationResult, state);
            }
            ImGui::EndChild();

            ImGui::SameLine();

            if (ImGui::BeginChild("FallbackCanvasInspectorPane", ImVec2(0, 0), true))
            {
                renderInspectorSection(doc, validationResult, state);
            }
            ImGui::EndChild();
        }

        ImGui::PopID();
    }

    void renderFallbackGraphViewer(const GraphDocument& doc, FallbackGraphViewerState& state,
                                   const ValidationOptions& validationOptions, const char* viewerId)
    {
        ValidationResult validationResult = validateGraph(doc, validationOptions);
        renderFallbackGraphViewer(doc, validationResult, state, viewerId);
    }

#else

    void renderFallbackGraphViewer(const GraphDocument& /*doc*/, const ValidationResult& /*validationResult*/,
                                   FallbackGraphViewerState& /*state*/, const char* /*viewerId*/)
    {
        // No-op in headless/non-ImGui builds
    }

    void renderFallbackGraphViewer(const GraphDocument& /*doc*/, FallbackGraphViewerState& /*state*/,
                                   const ValidationOptions& /*validationOptions*/, const char* /*viewerId*/)
    {
        // No-op in headless/non-ImGui builds
    }

#endif

} // namespace autoinput::ui::graph
