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
