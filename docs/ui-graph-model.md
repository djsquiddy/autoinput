# UI Graph Document Model

## Overview

The AutoInput UI Graph module (`autoinput::ui::graph`) provides an internal, dependency-free graph document representation and validation engine for future visual editors within the AutoInput UI.

> **Note**: This is an internal developer-facing graph data model and validation engine. It is not yet a user-visible feature in the application frontend.

## Goals and Design Principles

1. **Zero External Dependencies**: The graph document model and validation rules are completely decoupled from rendering toolkits and third-party node editor libraries (such as Dear ImGui, raylib, imnodes, or imgui-node-editor), as well as core automation playback engines.
2. **Deterministic and Testable**: The model and its validation engine can be constructed, manipulated, validated, serialized, and tested in pure unit test environments without initializing a graphic window or OpenGL/raylib backend.
3. **Stable Identifiers**: Elements use stable `NodeId`, `PinId`, and `LinkId` identifiers (`uint64_t`). ID lookups do not rely on container indices or vector ordering, guaranteeing stability during element deletions, additions, and validation passes.
4. **Visual Foundation**: Supports 2D coordinate positions (`NodePosition`) and metadata (`NodeMetadata`) for future node editors (such as sequence flow editors and configuration dependency visualizers).
5. **Configurable Validation Engine**: Provides topology and structural validation rules with customizable profiles for strict execution sequence graphs vs. permissive read-only configuration graphs.

## Core Structures

### `NodeKind`
Represents the functional role of a graph node:
- `Start`: Entrypoint trigger node.
- `End`: Terminal node.
- `RecordedEvent`: Raw recorded input event.
- `Wait`: Delay or timing pause.
- `Command`: Automated command execution.
- `Control`: Control flow branching or condition.
- `Input`: Synthetic input trigger.
- `Sequence`: Nested or chained sequence.
- `ExclusiveGroup`: Mutual exclusion container.
- `ApplicationFilter`: Target application focus allowlist filter.
- `BlacklistEntry`: Application focus exclusion rule.
- `Comment`: Visual annotation or note (ignored by execution flow validation).
- `Unknown`: Fallback / unrecognized node kind.

### `PinDirection`
Defines the connection direction of a node's pin:
- `Input`
- `Output`

### `NodePosition`
Stores 2D canvas coordinates:
```cpp
struct NodePosition
{
    float x{ 0.0F };
    float y{ 0.0F };
};
```

### `NodeMetadata`
Stores display and mapping information:
```cpp
struct NodeMetadata
{
    std::string title;
    std::string subtitle;
    std::optional<std::size_t> sourceIndex{ std::nullopt };
};
```

### `GraphPin`
Represents a connection endpoint on a node:
```cpp
struct GraphPin
{
    PinId id{ InvalidPinId };
    NodeId nodeId{ InvalidNodeId };
    PinDirection direction{ PinDirection::Input };
    std::string name;
};
```

### `GraphLink`
Represents a directed link connecting two pins:
```cpp
struct GraphLink
{
    LinkId id{ InvalidLinkId };
    PinId fromPinId{ InvalidPinId };
    PinId toPinId{ InvalidPinId };
};
```

### `GraphNode`
Represents a node in the graph document:
```cpp
struct GraphNode
{
    NodeId id{ InvalidNodeId };
    NodeKind kind{ NodeKind::Unknown };
    NodePosition position{ .x = 0.0F, .y = 0.0F };
    std::string title;
    std::string subtitle;
    std::optional<std::size_t> sourceIndex{ std::nullopt };
    std::vector<PinId> pinIds;
};
```

## GraphDocument API

The `GraphDocument` (aliased as `GraphModel`) manages the nodes, pins, and links:

### Node Management
- `createNode(kind, title, position)`: Creates a new node with an auto-generated unique ID.
- `createNodeWithId(id, kind, title, position)`: Creates a node with an explicit ID (e.g., during deserialization).
- `findNode(id)`: Looks up a node pointer by ID.
- `removeNode(id)`: Removes a node, automatically cleaning up all attached pins and connected links.

### Pin Management
- `createPin(nodeId, direction, name)`: Adds a pin to a node with an auto-generated unique ID.
- `createPinWithId(id, nodeId, direction, name)`: Adds a pin with an explicit ID.
- `findPin(id)`: Looks up a pin pointer by ID.
- `isPinOnNode(pinId, nodeId)` / `pinBelongsToNode(pinId, nodeId)`: Checks if a pin is owned by a given node.
- `removePin(id)`: Removes a pin and all links attached to it.

### Link Management
- `createLink(fromPinId, toPinId)`: Creates a connection between two valid pins.
- `createLinkWithId(id, fromPinId, toPinId)`: Creates a connection with an explicit link ID.
- `findLink(id)`: Looks up a link pointer by ID.
- `findLinkBetween(fromPinId, toPinId)`: Finds an existing link connecting two pins.
- `removeLink(id)`: Removes a connection by ID.
- `removeLinksConnectedToPin(pinId)`: Removes all links connected to a specific pin.

### Container & Lifecycle
- `nodes()`, `pins()`, `links()`: Const and mutable container access.
- `nodeCount()`, `pinCount()`, `linkCount()`: Element counts.
- `empty()`: Checks if the document contains no nodes.
- `clear()`: Empties all elements and resets internal ID allocators.

## Graph Validation Engine

Graph validation utilities are declared in `autoinput_ui/graph/graphValidator.h`.

### Severity Levels (`ValidationSeverity`)
- `Info`: Informational suggestions or status annotations.
- `Warning`: Non-fatal issues (e.g., disconnected / unused nodes in canvas).
- `Error`: Fatal structural issues preventing graph execution or compilation.

### Validation Finding (`ValidationIssue` / `ValidationMessage`)
Each finding includes:
- `severity`: `ValidationSeverity`
- `message`: Detailed diagnostic description
- `nodeId`: Optional `NodeId` indicating the affected node
- `linkId`: Optional `LinkId` indicating the affected link

### Configurable Profiles (`ValidationOptions`)
Validation rules are configurable via `ValidationOptions`:
- `requireStartNode`: Enforces presence of at least one `Start` node.
- `allowMultipleStartNodes`: Enforces uniqueness of `Start` node.
- `requireEndNode`: Enforces presence of at least one `End` node.
- `allowDisconnectedNodes`: Controls whether unlinked nodes emit warnings/errors.
- `requireAcyclic`: Runs cycle detection algorithms to ensure directed acyclic flow.
- `allowSelfLinks`: Prohibits links connecting pins on the same node.
- `treatDisconnectedAsError`: Escalates disconnected nodes from `Warning` to `Error`.

Presets:
- `ValidationOptions::sequenceGraph()`: Strict execution mode (requires single Start, at least one End, no cycles, no self-links, warns on disconnected nodes).
- `ValidationOptions::configGraph()`: Permissive visualization mode (allows multiple roots, optional start/end, allows cycles and disconnected nodes).

### Individual Rule Checkers
- `validateStartNodes(doc, requireStart, allowMultiple)`
- `validateEndNodes(doc, requireEnd)`
- `validateDisconnectedNodes(doc, severity)` (Comment nodes are automatically excluded)
- `validateLinkReferences(doc)` (Verifies pin existence)
- `validateLinkDirections(doc, allowSelfLinks)` (Detects invalid directions like output-to-output, input-to-input, input-to-output, and self-links)
- `validateAcyclic(doc)` (Directed cycle detection using DFS coloring)
- `validateGraph(doc, options)` (Comprehensive validation aggregation returning `ValidationResult`)

### Consuming Validation Messages in Visual Editors

Future visual node editors and graph viewers consume validation messages to provide real-time UI feedback:

1. **Node and Link Badging**: Visual editors look up `issue.nodeId` or `issue.linkId` to render visual error icons, warning badges, or highlight offending connection wires in red/amber.
2. **Inspector Problem Pane**: Display a structured list of issues sorted by severity, allowing users to double-click a message to pan the canvas camera directly to the relevant node or link.
3. **Execution Guarding**: Sequence playback and export routines query `result.isValid()` / `result.hasErrors()` to prevent compiling or executing structurally invalid sequence graphs.

## Sequence Graph Adapter

The Sequence Graph Adapter (`autoinput_ui/graph/sequenceGraphAdapter.h`) converts a recorded execution sequence (`autoinput::RecordedSequence`) into a `GraphDocument` representation for future visual node editors.

### Features
1. **Linear Topological Conversion**: Converts sequences into a linear execution graph connecting `Start -> Event 0 -> Event 1 -> ... -> End`.
2. **Source Index Preservation**: Every recorded event node retains its original 0-based vector index in `sourceIndex` so visual inspector panels can map edits directly back to the underlying `RecordedSequence`.
3. **Readable Node Metadata**: Generates informative titles and subtitles for all event types:
   - **Keyboard Events**: `Key Down` / `Key Up` with key labels.
   - **Mouse Button Events**: `Mouse Down` / `Mouse Up` with button identifiers.
   - **Mouse Move Events**: `Mouse Move` with coordinate positions.
   - **Invalid / Unknown Events**: Identified cleanly with diagnostic details.
4. **Flexible Timing Representation**:
   - By default (`separateWaitNodes = false`), delay intervals are embedded directly into the event's subtitle and metadata.
   - When `separateWaitNodes = true`, non-zero delays are automatically decomposed into distinct `Wait` nodes inserted into the chain before the target event.
5. **Deterministic Coordinate Layout**: Generates deterministic 2D node coordinates (`startX`, `startY`, `stepX`, `stepY`) enabling reproducible canvas positioning and unit test verification.
6. **Built-in Validation Compatibility**: Graphs generated by the adapter are guaranteed to pass strict `ValidationOptions::sequenceGraph()` rules.

### Sequence Conversion Example

```cpp
#include "autoinput_ui/graph/sequenceGraphAdapter.h"
#include "autoinput_ui/graph/graphValidator.h"

using namespace autoinput::ui::graph;

// Convert recorded sequence into a graph document
SequenceGraphOptions options;
options.separateWaitNodes = true; // Separate timing into distinct Wait nodes

GraphDocument doc = sequenceToGraphDocument(recordedSequence, options);

// Verify validity
ValidationResult result = validateGraph(doc, ValidationOptions::sequenceGraph());
assert(result.isValid());
```

## Sequence Graph Compiler

The Sequence Graph Compiler (`autoinput_ui/graph/sequenceGraphCompiler.h`) compiles a visual `GraphDocument` representation back into an `autoinput::RecordedSequence` data model.

### Compilation Workflow & Topology Rules

The compiler validates and reconstructs the sequence by tracing directed execution links:
1. **Single Entry & Single Exit**: Rejects graphs without exactly one `Start` node or without a reachable `End` node.
2. **Topological Ordering**: Uses graph links (rather than node storage order) to determine the exact execution sequence from `Start` to `End`.
3. **No Unsupported Branching / Merging**: Rejects graphs where execution nodes have multiple outgoing or incoming links.
4. **No Directed Cycles**: Rejects circular execution loops.
5. **No Disconnected Required Nodes**: Rejects graphs containing unlinked/unreachable execution nodes (`RecordedEvent`, `Wait`, `Start`, `End`).
6. **Valid Link Directions**: Rejects links with invalid pin directions (e.g. output-to-output or input-to-input).
7. **Metadata & Sequence Fidelity**: Preserves sequence-level properties (`name`, `start`, `repeat`) and source event details via `SequenceCompileOptions` (or an optional source sequence context).

### Current Limitations

- **Linear Sequences Only**: In this phase, the compiler only supports compiling linear sequence graphs (`Start -> Event... -> End`). Advanced control-flow constructs (conditionals, loops, branching) are rejected until the runtime sequence execution engine supports non-linear control flow.

### Sequence Compilation Example

```cpp
#include "autoinput_ui/graph/sequenceGraphCompiler.h"

using namespace autoinput::ui::graph;

// Compile a GraphDocument back into a RecordedSequence
SequenceCompileResult result = compileGraphToSequence(doc, originalSequence);

if (result.isSuccess())
{
    const RecordedSequence& compiled = *result.sequence;
    // Save to configuration or use in sequence player
}
else
{
    for (const auto& issue : result.issues)
    {
        // Display compilation error in problem inspector
    }
}
```

## Node Editor Backend Abstraction

The Node Editor Backend Abstraction (`autoinput_ui/graph/nodeEditorBackend.h`) provides a decoupled interface between AutoInput's graph document model and underlying immediate-mode graph rendering libraries.

### Core Interface (`INodeEditorBackend`)

The abstraction standardizes common visual canvas interactions without exposing third-party headers to public AutoInput consumers:

- **Lifecycle & Canvas Management**: `initialize()`, `shutdown()`, `beginCanvas(editorId)`, `endCanvas()`.
- **Node & Pin Layout**: `beginNode(nodeId)`, `endNode()`, `beginNodeTitle()`, `endNodeTitle()`, `beginInputPin(pinId)`, `endInputPin()`, `beginOutputPin(pinId)`, `endOutputPin()`.
- **Links & Topology**: `drawLink(linkId, startPinId, endPinId)`.
- **User Interaction Queries**: `queryCreatedLink()`, `queryDeletedLink()`, `querySelectedNodes()`, `querySelectedLinks()`.
- **Coordinate Synchronization**: `setNodePosition(nodeId, pos)`, `getNodePosition(nodeId)`.
- **Capability Introspection**: `capabilities()` reports `NodeEditorCapabilities` including support for canvas rendering, position queries, link creation/deletion events, selection queries, groups, comments, minimap, zoom, and multi-selection.

### Fallback Backend (`FallbackNodeEditorBackend`)

- The fallback backend is active by default in all standard builds and CI pipelines.
- Implements a completely dependency-free no-op implementation ensuring graph data models, validation, and conversion remain testable without third-party graphics dependencies.

### Optional `imnodes` Backend (`AUTOINPUT_UI_WITH_IMNODES`)

The project supports `imnodes` (by Nelarius) as an optional visual rendering backend for experimentation:

- **Disabled by Default**: `AUTOINPUT_UI_WITH_IMNODES` defaults to `OFF`. Default builds and CI do not require or download `imnodes`.
- **Enabling via CMake**:
  ```bash
  python scripts/commands/build.py ui -DAUTOINPUT_UI_WITH_IMNODES=ON -DIMNODES_DIR=/path/to/imnodes
  ```
- **Providing the Dependency Locally**:
  - Pass `-DIMNODES_DIR=/path/to/imnodes` pointing to the folder containing `imnodes.h` and `imnodes.cpp`.
  - Or place the repository directly in `third_party/imnodes` or `extern/imnodes`.
  - If `AUTOINPUT_UI_WITH_IMNODES` is `ON` and `imnodes` is not found, CMake will halt with an informative configuration error.
- **Backend Capabilities & Limitations**:
  - **Capabilities**: Full immediate-mode node rendering, title bars, input/output pins, connection linking, interactive link creation/destruction reporting, selection queries, and 2D canvas position synchronization.
  - **Limitations**: `imnodes` uses integer identifiers (`int`); public AutoInput `NodeId`/`PinId`/`LinkId` are cast accordingly. `imgui-node-editor` is not part of this backend.

## Fallback Graph Viewer (`FallbackGraphViewer`)

The Fallback Graph Viewer (`autoinput_ui/graph/fallbackGraphViewer.h`) provides a simple, dependency-free Dear ImGui visualization for `GraphDocument` instances without requiring `imnodes` or `imgui-node-editor`.

### Key Features

- **Multi-Panel Layout Modes**:
  - `Split`: Side-by-side view with topology list/canvas tabs on the left and inspector/validation on the right.
  - `ListOnly`: Compact tabular overview of nodes, links, inspector, and validation findings.
  - `Canvas`: Direct 2D interactive canvas preview rendered using Dear ImGui `ImDrawList` primitives.
- **Node & Link Visualization**:
  - Node headers with color-coded badges per `NodeKind` (e.g. Green for Start, Red for End, Blue for Events, Orange for Wait delays).
  - Displays titles, subtitles, source indices, and pin labels (`[In]` / `[Out]`).
  - Renders directional Bézier connection curves between linked pins.
- **Interactive Node & Link Selection**:
  - Click any node or link in the table, list, or 2D canvas to inspect its metadata and connections.
  - Selected elements are highlighted on the canvas and in tables with persistent state in `FallbackGraphViewerState`.
- **Integrated Validation Badging & Navigation**:
  - Displays validation status (`[Valid]` or error/warning counts) in the toolbar.
  - Attaches validation issue badges (`[!] Error`, `[!] Warn`) directly to affected nodes and links.
  - Clicking any validation message in the issues list immediately navigates to and selects the offending node or link.
- **Filtering & Search**:
  - Real-time text filter matching node titles, subtitles, IDs, and kind strings.
  - Toggle to filter exclusively to problematic nodes with validation errors/warnings.

### Limitations

- **Read-Only Inspection**: The fallback viewer is primarily designed for graph visualization, inspection, and validation triage; it does not support interactive drag-and-drop link creation or pin disconnection (which will be handled by dedicated visual node editor backends).
- **Basic 2D Canvas**: Canvas rendering is a lightweight preview using Dear ImGui draw lists; complex node graphs with hundreds of nodes should be viewed using list mode or a dedicated node editor backend.

### Viewer Usage Example

```cpp
#include "autoinput_ui/graph/fallbackGraphViewer.h"
#include "autoinput_ui/graph/graphValidator.h"

using namespace autoinput::ui::graph;

FallbackGraphViewerState viewerState;

// Render fallback viewer with automatic validation
renderFallbackGraphViewer(doc, viewerState);
```

## Usage Example

```cpp
#include "autoinput_ui/graph/graphModel.h"
#include "autoinput_ui/graph/graphValidator.h"

using namespace autoinput::ui::graph;

GraphDocument graph;

// Create nodes
auto& startNode = graph.createNode(NodeKind::Start, "Start Trigger", { .x = 100.0F, .y = 150.0F });
auto& waitNode  = graph.createNode(NodeKind::Wait, "Wait 500ms", { .x = 300.0F, .y = 150.0F });
auto& endNode   = graph.createNode(NodeKind::End, "End", { .x = 500.0F, .y = 150.0F });

// Add pins
auto* startOut = graph.createPin(startNode.id, PinDirection::Output, "Out");
auto* waitIn   = graph.createPin(waitNode.id, PinDirection::Input, "In");
auto* waitOut  = graph.createPin(waitNode.id, PinDirection::Output, "Out");
auto* endIn    = graph.createPin(endNode.id, PinDirection::Input, "In");

// Connect pins with links
graph.createLink(startOut->id, waitIn->id);
graph.createLink(waitOut->id, endIn->id);

// Validate graph for sequence execution
const ValidationResult result = validateGraph(graph, ValidationOptions::sequenceGraph());

if (result.isValid())
{
    // Graph is structurally sound and ready for playback/export
}
else
{
    for (const auto& issue : result.issues)
    {
        // Display issue in editor problem pane and badge affected node/link
    }
}
```
