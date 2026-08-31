/**
 * @file graphLayoutMetadata.h
 * @brief Optional persistence and serialization for visual graph editor layout metadata.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_GRAPH_GRAPH_LAYOUT_METADATA_H
#define INCLUDE_AUTOINPUT_UI_GRAPH_GRAPH_LAYOUT_METADATA_H

#include "autoinput/config/config.h"
#include "autoinput_ui/graph/graphModel.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace autoinput::ui::graph
{
    /**
     * @brief Current version of the graph layout metadata schema.
     */
    inline constexpr std::uint32_t CurrentGraphMetadataVersion{ 1 };

    /**
     * @brief Format identifier for graph layout metadata files.
     */
    inline constexpr std::string_view GraphMetadataFormatIdentifier{ "autoinput-graph-layout" };

    /**
     * @brief File extension for standalone graph layout sidecar files.
     */
    inline constexpr std::string_view GraphMetadataFileExtension{ ".graph.toml" };

    /**
     * @brief Node layout data representing visual-only properties for a specific node.
     */
    struct NodeLayoutData
    {
        std::string nodeKey;
        NodePosition position{ .x = 0.0F, .y = 0.0F };
        bool collapsed{ false };
        std::optional<std::string> comment{ std::nullopt };

        constexpr auto operator<=>(const NodeLayoutData&) const = default;
    };

    /**
     * @brief Viewport and filter settings for a graph canvas.
     */
    struct GraphLayoutViewSettings
    {
        NodePosition offset{ .x = 0.0F, .y = 0.0F };
        float zoom{ 1.0F };

        // Filter toggles (used for config graph viewer)
        bool showCommands{ true };
        bool showControls{ true };
        bool showSequences{ true };
        bool showInputs{ true };
        bool showExclusiveGroups{ true };
        bool showGlobalSettings{ true };
        bool showBlacklist{ true };

        constexpr auto operator<=>(const GraphLayoutViewSettings&) const = default;
    };

    /**
     * @brief Layout document for a single graph canvas (e.g. sequence editor or config viewer).
     */
    struct GraphLayoutDocument
    {
        std::uint32_t version{ CurrentGraphMetadataVersion };
        std::string graphName;
        GraphLayoutViewSettings viewSettings;
        std::map<std::string, NodeLayoutData, std::less<>> nodes;

        void setNodeLayout(std::string_view key, NodePosition position, bool collapsed = false);
        [[nodiscard]] const NodeLayoutData* findNodeLayout(std::string_view key) const noexcept;
        [[nodiscard]] NodeLayoutData* findNodeLayout(std::string_view key) noexcept;
        bool removeNodeLayout(std::string_view key);
        [[nodiscard]] std::size_t nodeCount() const noexcept { return nodes.size(); }
        void clear() noexcept { nodes.clear(); }
    };

    /**
     * @brief Container document holding layout metadata for an entire configuration file.
     *
     * Stores layout data for the top-level configuration relationship graph,
     * as well as individual layout data for each named sequence graph.
     */
    struct ConfigGraphMetadataDocument
    {
        std::uint32_t version{ CurrentGraphMetadataVersion };
        std::string format{ GraphMetadataFormatIdentifier };
        GraphLayoutDocument configGraphLayout;
        std::map<std::string, GraphLayoutDocument, std::less<>> sequenceGraphLayouts;

        [[nodiscard]] GraphLayoutDocument& getOrCreateSequenceLayout(std::string_view sequenceName);
        [[nodiscard]] const GraphLayoutDocument* findSequenceLayout(std::string_view sequenceName) const noexcept;
        [[nodiscard]] GraphLayoutDocument* findSequenceLayout(std::string_view sequenceName) noexcept;
        bool removeSequenceLayout(std::string_view sequenceName);
    };

    // --- Stable Key and Mapping Functions ---

    /**
     * @brief Computes a deterministic stable string key for a graph node based on its kind, title, and source index.
     *
     * Key formats:
     * - Start: "start"
     * - End: "end"
     * - RecordedEvent: "event:<sourceIndex>" (or "event:title")
     * - Wait: "wait:<sourceIndex>"
     * - Command: "cmd:<title>"
     * - Control: "ctrl:<title>:<sourceIndex>"
     * - Sequence: "seq:<title>"
     * - Input: "input:<title>"
     * - ExclusiveGroup: "group:<title>"
     * - ApplicationFilter: "filter:app"
     * - BlacklistEntry: "blacklist:<sourceIndex>"
     * - Comment: "comment:<id>"
     * - Unknown: "node:<id>"
     */
    [[nodiscard]] std::string getNodeStableKey(const GraphNode& node);

    /**
     * @brief Applies stored layout positions and properties from metadata onto a GraphDocument.
     *
     * Nodes in the graph whose stable keys match entries in the layout document will have their positions updated.
     * Nodes not present in the layout document retain their existing layout positions.
     *
     * @return Number of nodes whose positions were updated.
     */
    std::size_t applyLayoutToGraph(GraphDocument& graph, const GraphLayoutDocument& layout);

    /**
     * @brief Extracts a GraphLayoutDocument containing node positions and view settings from a GraphDocument.
     */
    [[nodiscard]] GraphLayoutDocument extractLayoutFromGraph(const GraphDocument& graph,
                                                             const GraphLayoutViewSettings& viewSettings = {},
                                                             std::string_view graphName = {});

    // --- Cleanup and Pruning Functions ---

    /**
     * @brief Prunes stale node layout entries whose keys are no longer present in the given GraphDocument.
     * @return Number of stale node entries removed.
     */
    std::size_t pruneStaleMetadata(GraphLayoutDocument& layout, const GraphDocument& currentGraph);

    /**
     * @brief Prunes stale event/wait entries from a sequence layout when events have been deleted.
     * @return Number of stale entries removed.
     */
    std::size_t pruneStaleMetadata(GraphLayoutDocument& layout, const autoinput::RecordedSequence& sequence);

    /**
     * @brief Prunes stale commands, controls, sequences, and blacklist entries across an entire config metadata document.
     * @return Total number of stale entries/layouts removed.
     */
    std::size_t pruneStaleMetadata(ConfigGraphMetadataDocument& doc, const autoinput::ConfigData& config);

    // --- TOML Serialization and Deserialization ---

    /**
     * @brief Serializes a full configuration graph metadata document into a TOML string.
     */
    [[nodiscard]] std::string serializeGraphMetadataToToml(const ConfigGraphMetadataDocument& doc);

    /**
     * @brief Serializes a single graph layout document into a TOML string.
     */
    [[nodiscard]] std::string serializeGraphLayoutToToml(const GraphLayoutDocument& layout);

    /**
     * @brief Deserializes a full configuration graph metadata document from a TOML string.
     *
     * Handles versioning gracefully. If the version is newer than supported, attempts best-effort parsing
     * or logs an informative warning message.
     */
    [[nodiscard]] std::optional<ConfigGraphMetadataDocument> deserializeGraphMetadataFromToml(
        std::string_view tomlContent, std::string* outErrorMessage = nullptr);

    /**
     * @brief Deserializes a single graph layout document from a TOML string.
     */
    [[nodiscard]] std::optional<GraphLayoutDocument> deserializeGraphLayoutFromToml(
        std::string_view tomlContent, std::string* outErrorMessage = nullptr);

    // --- File Persistence Helpers ---

    /**
     * @brief Generates the canonical sidecar metadata path for a configuration file.
     *
     * Example: "configs/macro.toml" -> "configs/macro.graph.toml"
     */
    [[nodiscard]] std::filesystem::path getGraphMetadataPathForConfig(const std::filesystem::path& configPath);

    /**
     * @brief Saves a configuration graph metadata document to a sidecar file.
     * @return True if successfully written.
     */
    bool saveGraphMetadataFile(const std::filesystem::path& metadataPath, const ConfigGraphMetadataDocument& doc);

    /**
     * @brief Loads a configuration graph metadata document from a specific file path.
     * @return Metadata document if file exists and parses, std::nullopt otherwise.
     */
    [[nodiscard]] std::optional<ConfigGraphMetadataDocument> loadGraphMetadataFile(
        const std::filesystem::path& metadataPath);

    /**
     * @brief Loads graph layout metadata for a given configuration file, if the sidecar file exists.
     *
     * If the sidecar file is missing or corrupted, returns std::nullopt without throwing or affecting the config.
     */
    [[nodiscard]] std::optional<ConfigGraphMetadataDocument> loadGraphMetadataForConfig(
        const std::filesystem::path& configPath);

} // namespace autoinput::ui::graph

#endif // INCLUDE_AUTOINPUT_UI_GRAPH_GRAPH_LAYOUT_METADATA_H
