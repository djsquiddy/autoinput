/**
 * @file graphLayoutMetadata.cpp
 * @brief Implementation of optional persistence for visual graph editor layout metadata.
 * @author djsquiddy
 * @date August 2026
 */
#include "graphLayoutMetadata.h"

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>
#include <unordered_set>

#if defined(__cpp_exceptions) && __cpp_exceptions
#define TOML_EXCEPTIONS 0
#endif
#include <toml++/toml.hpp>

namespace autoinput::ui::graph
{
    namespace
    {
        namespace fs = std::filesystem;

        template <typename T>
        void tryGetTomlValue(const toml::table& tbl, std::string_view key, T& outVal)
        {
            if (const auto node = tbl[key])
            {
                if constexpr (std::is_same_v<T, std::string>)
                {
                    if (node.is_string())
                    {
                        outVal = node.as_string()->get();
                    }
                }
                else if constexpr (std::is_same_v<T, float>)
                {
                    if (node.is_number())
                    {
                        outVal = static_cast<float>(node.as_floating_point()
                                                        ? node.as_floating_point()->get()
                                                        : static_cast<double>(node.as_integer()->get()));
                    }
                }
                else if constexpr (std::is_same_v<T, bool>)
                {
                    if (node.is_boolean())
                    {
                        outVal = node.as_boolean()->get();
                    }
                }
                else if constexpr (std::is_same_v<T, std::uint32_t>)
                {
                    if (node.is_integer())
                    {
                        outVal = static_cast<std::uint32_t>(node.as_integer()->get());
                    }
                }
            }
        }

        void populateViewSettings(GraphLayoutViewSettings& settings, const toml::table& tbl)
        {
            tryGetTomlValue(tbl, "offset_x", settings.offset.x);
            tryGetTomlValue(tbl, "offset_y", settings.offset.y);
            tryGetTomlValue(tbl, "zoom", settings.zoom);
        }

        void populateFilterSettings(GraphLayoutViewSettings& settings, const toml::table& tbl)
        {
            tryGetTomlValue(tbl, "show_commands", settings.showCommands);
            tryGetTomlValue(tbl, "show_controls", settings.showControls);
            tryGetTomlValue(tbl, "show_sequences", settings.showSequences);
            tryGetTomlValue(tbl, "show_inputs", settings.showInputs);
            tryGetTomlValue(tbl, "show_exclusive_groups", settings.showExclusiveGroups);
            tryGetTomlValue(tbl, "show_global_settings", settings.showGlobalSettings);
            tryGetTomlValue(tbl, "show_blacklist", settings.showBlacklist);
        }

        void populateNodes(std::map<std::string, NodeLayoutData, std::less<>>& nodes, const toml::table& tbl)
        {
            for (const auto& [nodeKey, valNode] : tbl)
            {
                NodeLayoutData layoutData;
                layoutData.nodeKey = std::string(nodeKey.str());
                if (const auto* valTbl = valNode.as_table())
                {
                    tryGetTomlValue(*valTbl, "x", layoutData.position.x);
                    tryGetTomlValue(*valTbl, "y", layoutData.position.y);
                    tryGetTomlValue(*valTbl, "collapsed", layoutData.collapsed);
                    std::string commentStr;
                    tryGetTomlValue(*valTbl, "comment", commentStr);
                    if (!commentStr.empty())
                    {
                        layoutData.comment = std::move(commentStr);
                    }
                }
                nodes[layoutData.nodeKey] = std::move(layoutData);
            }
        }

        void populateGraphLayoutDocument(GraphLayoutDocument& doc, const toml::table& tbl)
        {
            tryGetTomlValue(tbl, "version", doc.version);
            tryGetTomlValue(tbl, "name", doc.graphName);

            if (const auto* viewTbl = tbl["view"].as_table())
            {
                populateViewSettings(doc.viewSettings, *viewTbl);
            }
            if (const auto* filterTbl = tbl["filters"].as_table())
            {
                populateFilterSettings(doc.viewSettings, *filterTbl);
            }
            if (const auto* nodesTbl = tbl["nodes"].as_table())
            {
                populateNodes(doc.nodes, *nodesTbl);
            }
        }

        toml::table createViewTable(const GraphLayoutViewSettings& settings)
        {
            toml::table t;
            t.is_inline(true);
            t.insert("offset_x", static_cast<double>(settings.offset.x));
            t.insert("offset_y", static_cast<double>(settings.offset.y));
            t.insert("zoom", static_cast<double>(settings.zoom));
            return t;
        }

        toml::table createFiltersTable(const GraphLayoutViewSettings& settings)
        {
            toml::table t;
            t.is_inline(true);
            t.insert("show_commands", settings.showCommands);
            t.insert("show_controls", settings.showControls);
            t.insert("show_sequences", settings.showSequences);
            t.insert("show_inputs", settings.showInputs);
            t.insert("show_exclusive_groups", settings.showExclusiveGroups);
            t.insert("show_global_settings", settings.showGlobalSettings);
            t.insert("show_blacklist", settings.showBlacklist);
            return t;
        }

        toml::table createNodesTable(const std::map<std::string, NodeLayoutData, std::less<>>& nodes)
        {
            toml::table t;
            for (const auto& [key, data] : nodes)
            {
                toml::table nodeTbl;
                nodeTbl.is_inline(true);
                nodeTbl.insert("x", static_cast<double>(data.position.x));
                nodeTbl.insert("y", static_cast<double>(data.position.y));
                if (data.collapsed)
                {
                    nodeTbl.insert("collapsed", true);
                }
                if (data.comment.has_value() && !data.comment->empty())
                {
                    nodeTbl.insert("comment", *data.comment);
                }
                t.insert(key, std::move(nodeTbl));
            }
            return t;
        }

        toml::table createGraphLayoutTable(const GraphLayoutDocument& layout)
        {
            toml::table t;
            t.insert("version", static_cast<std::int64_t>(layout.version));
            if (!layout.graphName.empty())
            {
                t.insert("name", layout.graphName);
            }
            t.insert("view", createViewTable(layout.viewSettings));
            t.insert("filters", createFiltersTable(layout.viewSettings));
            t.insert("nodes", createNodesTable(layout.nodes));
            return t;
        }

    } // namespace

    // =========================================================================
    // GraphLayoutDocument Implementation
    // =========================================================================

    void GraphLayoutDocument::setNodeLayout(std::string_view key, NodePosition position, bool collapsed)
    {
        nodes[std::string(key)] =
            NodeLayoutData{ .nodeKey = std::string(key), .position = position, .collapsed = collapsed };
    }

    const NodeLayoutData* GraphLayoutDocument::findNodeLayout(std::string_view key) const noexcept
    {
        const auto it = nodes.find(key);
        if (it != nodes.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    NodeLayoutData* GraphLayoutDocument::findNodeLayout(std::string_view key) noexcept
    {
        const auto it = nodes.find(key);
        if (it != nodes.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    bool GraphLayoutDocument::removeNodeLayout(std::string_view key)
    {
        return nodes.erase(key) > 0;
    }

    // =========================================================================
    // ConfigGraphMetadataDocument Implementation
    // =========================================================================

    GraphLayoutDocument& ConfigGraphMetadataDocument::getOrCreateSequenceLayout(std::string_view sequenceName)
    {
        auto it = sequenceGraphLayouts.find(sequenceName);
        if (it == sequenceGraphLayouts.end())
        {
            GraphLayoutDocument layout;
            layout.version = version;
            layout.graphName = std::string(sequenceName);
            it = sequenceGraphLayouts.emplace(std::string(sequenceName), std::move(layout)).first;
        }
        return it->second;
    }

    const GraphLayoutDocument* ConfigGraphMetadataDocument::findSequenceLayout(
        std::string_view sequenceName) const noexcept
    {
        const auto it = sequenceGraphLayouts.find(sequenceName);
        if (it != sequenceGraphLayouts.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    GraphLayoutDocument* ConfigGraphMetadataDocument::findSequenceLayout(std::string_view sequenceName) noexcept
    {
        const auto it = sequenceGraphLayouts.find(sequenceName);
        if (it != sequenceGraphLayouts.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    bool ConfigGraphMetadataDocument::removeSequenceLayout(std::string_view sequenceName)
    {
        return sequenceGraphLayouts.erase(sequenceName) > 0;
    }

    // =========================================================================
    // Stable Key and Mapping Functions
    // =========================================================================

    std::string getNodeStableKey(const GraphNode& node)
    {
        switch (node.kind)
        {
        case NodeKind::Start: return "start";
        case NodeKind::End:
            if (node.title == "Global End" || node.title == "End Key")
            {
                return "global:end";
            }
            return "end";
        case NodeKind::RecordedEvent:
            if (node.sourceIndex.has_value())
            {
                return std::format("event:{}", *node.sourceIndex);
            }
            return std::format("event:node_{}", node.id);
        case NodeKind::Wait:
            if (node.sourceIndex.has_value())
            {
                return std::format("wait:{}", *node.sourceIndex);
            }
            return std::format("wait:node_{}", node.id);
        case NodeKind::Command:
            if (!node.title.empty())
            {
                return std::format("cmd:{}", node.title);
            }
            if (node.sourceIndex.has_value())
            {
                return std::format("cmd:index_{}", *node.sourceIndex);
            }
            return std::format("cmd:node_{}", node.id);
        case NodeKind::Control:
            if (node.sourceIndex.has_value() && !node.title.empty())
            {
                return std::format("ctrl:{}:{}", node.title, *node.sourceIndex);
            }
            if (!node.title.empty())
            {
                return std::format("ctrl:{}", node.title);
            }
            return std::format("ctrl:node_{}", node.id);
        case NodeKind::Sequence:
            if (!node.title.empty())
            {
                return std::format("seq:{}", node.title);
            }
            if (node.sourceIndex.has_value())
            {
                return std::format("seq:index_{}", *node.sourceIndex);
            }
            return std::format("seq:node_{}", node.id);
        case NodeKind::Input:
            if (!node.title.empty())
            {
                return std::format("input:{}", node.title);
            }
            return std::format("input:node_{}", node.id);
        case NodeKind::ExclusiveGroup:
            if (!node.title.empty())
            {
                return std::format("group:{}", node.title);
            }
            return std::format("group:node_{}", node.id);
        case NodeKind::ApplicationFilter: return "filter:app";
        case NodeKind::BlacklistEntry:
            if (node.sourceIndex.has_value())
            {
                return std::format("blacklist:{}", *node.sourceIndex);
            }
            return std::format("blacklist:node_{}", node.id);
        case NodeKind::Comment: return std::format("comment:{}", node.id);
        case NodeKind::Unknown:
        default: return std::format("node:{}", node.id);
        }
    }

    std::size_t applyLayoutToGraph(GraphDocument& graph, const GraphLayoutDocument& layout)
    {
        std::size_t appliedCount = 0;
        for (auto& node : graph.nodes())
        {
            const std::string key = getNodeStableKey(node);
            if (const auto* entry = layout.findNodeLayout(key))
            {
                node.position = entry->position;
                appliedCount++;
            }
        }
        return appliedCount;
    }

    GraphLayoutDocument extractLayoutFromGraph(const GraphDocument& graph, const GraphLayoutViewSettings& viewSettings,
                                               std::string_view graphName)
    {
        GraphLayoutDocument doc;
        doc.version = CurrentGraphMetadataVersion;
        doc.graphName = std::string(graphName);
        doc.viewSettings = viewSettings;

        for (const auto& node : graph.nodes())
        {
            NodeLayoutData entry;
            entry.nodeKey = getNodeStableKey(node);
            entry.position = node.position;
            entry.collapsed = false;
            doc.nodes[entry.nodeKey] = std::move(entry);
        }
        return doc;
    }

    // =========================================================================
    // Cleanup and Pruning Functions
    // =========================================================================

    std::size_t pruneStaleMetadata(GraphLayoutDocument& layout, const GraphDocument& currentGraph)
    {
        std::unordered_set<std::string> validKeys;
        for (const auto& node : currentGraph.nodes())
        {
            validKeys.insert(getNodeStableKey(node));
        }

        std::size_t removedCount = 0;
        for (auto it = layout.nodes.begin(); it != layout.nodes.end();)
        {
            if (!validKeys.contains(it->first))
            {
                it = layout.nodes.erase(it);
                removedCount++;
            }
            else
            {
                ++it;
            }
        }
        return removedCount;
    }

    std::size_t pruneStaleMetadata(GraphLayoutDocument& layout, const autoinput::RecordedSequence& sequence)
    {
        std::unordered_set<std::string> validKeys;
        validKeys.insert("start");
        validKeys.insert("end");

        for (std::size_t i = 0; i < sequence.events.size(); ++i)
        {
            validKeys.insert(std::format("event:{}", i));
            validKeys.insert(std::format("wait:{}", i));
        }

        std::size_t removedCount = 0;
        for (auto it = layout.nodes.begin(); it != layout.nodes.end();)
        {
            // If it's an event or wait key that is outside the valid range, prune it
            if ((it->first.starts_with("event:") || it->first.starts_with("wait:")) && !validKeys.contains(it->first))
            {
                it = layout.nodes.erase(it);
                removedCount++;
            }
            else
            {
                ++it;
            }
        }
        return removedCount;
    }

    std::size_t pruneStaleMetadata(ConfigGraphMetadataDocument& doc, const autoinput::ConfigData& config)
    {
        std::size_t totalRemoved = 0;

        // 1. Build valid keys for the config relationship graph
        std::unordered_set<std::string> validConfigKeys;
        validConfigKeys.insert("global:end");
        validConfigKeys.insert("end");
        validConfigKeys.insert("filter:app");

        for (std::size_t i = 0; i < config.blacklist.size(); ++i)
        {
            validConfigKeys.insert(std::format("blacklist:{}", i));
        }

        for (const auto& cmd : config.commands)
        {
            if (!cmd.name.empty())
            {
                validConfigKeys.insert(std::format("cmd:{}", cmd.name));
            }
            if (!cmd.exclusiveGroup.empty())
            {
                validConfigKeys.insert(std::format("group:{}", cmd.exclusiveGroup));
            }
            for (std::size_t ci = 0; ci < cmd.controls.size(); ++ci)
            {
                if (!cmd.name.empty())
                {
                    validConfigKeys.insert(std::format("ctrl:{}:{}", cmd.name, ci));
                }
            }
            for (const auto& key : cmd.keys)
            {
                validConfigKeys.insert(std::format("input:{}", key));
            }
            for (const auto& skey : cmd.startKeys)
            {
                validConfigKeys.insert(std::format("input:{}", skey));
            }
            for (const auto& btn : cmd.buttons)
            {
                validConfigKeys.insert(std::format("input:{}", btn));
            }
        }

        std::unordered_set<std::string> validSequenceNames;
        for (const auto& seq : config.sequences)
        {
            if (!seq.name.empty())
            {
                validConfigKeys.insert(std::format("seq:{}", seq.name));
                validSequenceNames.insert(seq.name);
            }
            if (!seq.start.empty())
            {
                validConfigKeys.insert(std::format("input:{}", seq.start));
            }
        }

        // Prune config graph layout nodes
        for (auto it = doc.configGraphLayout.nodes.begin(); it != doc.configGraphLayout.nodes.end();)
        {
            if (!validConfigKeys.contains(it->first))
            {
                it = doc.configGraphLayout.nodes.erase(it);
                totalRemoved++;
            }
            else
            {
                ++it;
            }
        }

        // 2. Prune sequence layouts
        for (auto it = doc.sequenceGraphLayouts.begin(); it != doc.sequenceGraphLayouts.end();)
        {
            if (!validSequenceNames.contains(it->first))
            {
                it = doc.sequenceGraphLayouts.erase(it);
                totalRemoved++;
            }
            else
            {
                // Find matching sequence and prune internal event/wait nodes
                const auto seqIt =
                    std::ranges::find_if(config.sequences, [&](const auto& s) { return s.name == it->first; });
                if (seqIt != config.sequences.end())
                {
                    totalRemoved += pruneStaleMetadata(it->second, *seqIt);
                }
                ++it;
            }
        }

        return totalRemoved;
    }

    // =========================================================================
    // TOML Serialization and Deserialization
    // =========================================================================

    std::string serializeGraphLayoutToToml(const GraphLayoutDocument& layout)
    {
        toml::table root = createGraphLayoutTable(layout);
        std::stringstream ss;
        ss << root;
        return ss.str();
    }

    std::string serializeGraphMetadataToToml(const ConfigGraphMetadataDocument& doc)
    {
        toml::table root;
        toml::table metaTbl;
        metaTbl.is_inline(true);
        metaTbl.insert("version", static_cast<std::int64_t>(doc.version));
        metaTbl.insert("format", doc.format);
        root.insert("metadata", std::move(metaTbl));

        // Config graph layout table
        root.insert("config_graph", createGraphLayoutTable(doc.configGraphLayout));

        // Sequence graph layouts
        if (!doc.sequenceGraphLayouts.empty())
        {
            toml::table sequencesTbl;
            for (const auto& [seqName, seqLayout] : doc.sequenceGraphLayouts)
            {
                sequencesTbl.insert(seqName, createGraphLayoutTable(seqLayout));
            }
            root.insert("sequences", std::move(sequencesTbl));
        }

        std::stringstream ss;
        ss << root;
        return ss.str();
    }

    std::optional<GraphLayoutDocument> deserializeGraphLayoutFromToml(std::string_view tomlContent,
                                                                      std::string* outErrorMessage)
    {
        if (tomlContent.empty())
        {
            if (outErrorMessage)
            {
                *outErrorMessage = "Empty TOML content";
            }
            return std::nullopt;
        }

        const auto parseResult = toml::parse(tomlContent);
        if (!parseResult)
        {
            if (outErrorMessage)
            {
                *outErrorMessage = std::string(parseResult.error().description());
            }
            return std::nullopt;
        }

        GraphLayoutDocument doc;
        populateGraphLayoutDocument(doc, parseResult.table());
        return doc;
    }

    std::optional<ConfigGraphMetadataDocument> deserializeGraphMetadataFromToml(std::string_view tomlContent,
                                                                                std::string* outErrorMessage)
    {
        if (tomlContent.empty())
        {
            if (outErrorMessage)
            {
                *outErrorMessage = "Empty TOML content";
            }
            return std::nullopt;
        }

        const auto parseResult = toml::parse(tomlContent);
        if (!parseResult)
        {
            if (outErrorMessage)
            {
                *outErrorMessage = std::string(parseResult.error().description());
            }
            return std::nullopt;
        }

        const auto& rootTable = parseResult.table();
        ConfigGraphMetadataDocument doc;

        if (const auto* metaTbl = rootTable["metadata"].as_table())
        {
            tryGetTomlValue(*metaTbl, "version", doc.version);
            tryGetTomlValue(*metaTbl, "format", doc.format);
        }
        else
        {
            tryGetTomlValue(rootTable, "version", doc.version);
            tryGetTomlValue(rootTable, "format", doc.format);
        }

        // Config graph layout
        if (const auto cfgNode = rootTable["config_graph"])
        {
            if (const auto* cfgTbl = cfgNode.as_table())
            {
                populateGraphLayoutDocument(doc.configGraphLayout, *cfgTbl);
            }
        }
        else
        {
            // Support flat graph layout format in root table for convenience
            populateGraphLayoutDocument(doc.configGraphLayout, rootTable);
        }

        // Sequence graph layouts
        if (const auto seqsNode = rootTable["sequences"])
        {
            if (const auto* seqsTbl = seqsNode.as_table())
            {
                for (const auto& [seqName, valNode] : *seqsTbl)
                {
                    if (const auto* seqLayoutTbl = valNode.as_table())
                    {
                        GraphLayoutDocument seqLayout;
                        populateGraphLayoutDocument(seqLayout, *seqLayoutTbl);
                        seqLayout.graphName = std::string(seqName.str());
                        doc.sequenceGraphLayouts[seqLayout.graphName] = std::move(seqLayout);
                    }
                }
            }
        }

        return doc;
    }

    // =========================================================================
    // File Persistence Helpers
    // =========================================================================

    std::filesystem::path getGraphMetadataPathForConfig(const std::filesystem::path& configPath)
    {
        if (configPath.empty())
        {
            return {};
        }

        const auto filename = configPath.filename().string();
        if (filename.ends_with(GraphMetadataFileExtension))
        {
            return configPath;
        }

        const auto stem = configPath.stem().string();
        const auto parent = configPath.parent_path();
        const auto metadataFilename = stem + std::string(GraphMetadataFileExtension);

        if (parent.empty())
        {
            return fs::path(metadataFilename);
        }
        return parent / metadataFilename;
    }

    bool saveGraphMetadataFile(const std::filesystem::path& metadataPath, const ConfigGraphMetadataDocument& doc)
    {
        if (metadataPath.empty())
        {
            return false;
        }

        try
        {
            const auto parent = metadataPath.parent_path();
            if (!parent.empty() && !fs::exists(parent))
            {
                fs::create_directories(parent);
            }

            std::ofstream file(metadataPath, std::ios::out | std::ios::trunc);
            if (!file.is_open())
            {
                return false;
            }

            file << serializeGraphMetadataToToml(doc);
            return file.good();
        }
        catch (...)
        {
            return false;
        }
    }

    std::optional<ConfigGraphMetadataDocument> loadGraphMetadataFile(const std::filesystem::path& metadataPath)
    {
        if (metadataPath.empty() || !fs::exists(metadataPath) || !fs::is_regular_file(metadataPath))
        {
            return std::nullopt;
        }

        try
        {
            std::ifstream file(metadataPath);
            if (!file.is_open())
            {
                return std::nullopt;
            }

            std::stringstream ss;
            ss << file.rdbuf();
            return deserializeGraphMetadataFromToml(ss.str());
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    std::optional<ConfigGraphMetadataDocument> loadGraphMetadataForConfig(const std::filesystem::path& configPath)
    {
        const auto metadataPath = getGraphMetadataPathForConfig(configPath);
        return loadGraphMetadataFile(metadataPath);
    }

} // namespace autoinput::ui::graph
