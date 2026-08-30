# UI Graph Document Model

## Overview

The AutoInput UI Graph module (`autoinput::ui::graph`) provides an internal, dependency-free graph document representation for future visual editors within the AutoInput UI.

> **Note**: This is an internal developer-facing graph data model. It is not yet a user-visible feature in the application frontend.

## Goals and Design Principles

1. **Zero External Dependencies**: The graph document model is completely decoupled from rendering toolkits and third-party node editor libraries (such as Dear ImGui, raylib, imnodes, or imgui-node-editor), as well as core automation playback engines.
2. **Deterministic and Testable**: The model can be constructed, manipulated, validated, serialized, and tested in pure unit test environments without initializing a graphic window or OpenGL/raylib backend.
3. **Stable Identifiers**: Elements use stable `NodeId`, `PinId`, and `LinkId` identifiers (`uint64_t`). ID lookups do not rely on container indices or vector ordering, guaranteeing stability during element deletions and additions.
4. **Visual Foundation**: Supports 2D coordinate positions (`NodePosition`) and metadata (`NodeMetadata`) for future node editors (such as sequence flow editors and configuration dependency visualizers).

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
- `Comment`: Visual annotation or note.
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

## Usage Example

```cpp
#include "autoinput_ui/graph/graphModel.h"

using namespace autoinput::ui::graph;

GraphDocument graph;

// Create nodes
auto& startNode = graph.createNode(NodeKind::Start, "Start Trigger", { .x = 100.0F, .y = 150.0F });
auto& waitNode  = graph.createNode(NodeKind::Wait, "Wait 500ms", { .x = 300.0F, .y = 150.0F });

// Add pins
auto* startOut = graph.createPin(startNode.id, PinDirection::Output, "Out");
auto* waitIn   = graph.createPin(waitNode.id, PinDirection::Input, "In");

// Connect pins with a link
auto* link = graph.createLink(startOut->id, waitIn->id);

// Removing a node cleans up attached pins and links
graph.removeNode(startNode.id);
// startOut pin and the connecting link are now automatically removed
```
