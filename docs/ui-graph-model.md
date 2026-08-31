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

## Sequence Graph Viewer / Editor UI (`SequenceGraphEditor`)

The Sequence Graph Editor (`autoinput_ui/editors/sequenceGraphEditor.h`) provides an interactive UI component and non-UI state controller exposing visual sequence inspection, linear graph editing, validation, and safe sequence compilation directly within the AutoInput UI.

### Key Capabilities & Workflow

- **Sequence Visualization & Synchronization**:
  - Automatically converts active `RecordedSequence` data into a linear `GraphDocument` representation (`Start -> RecordedEvent / Wait -> End`).
  - Integrated into `SequenceEditorWindow` with a tabbed interface allowing instant switching between the tabular `Steps Table` and the `Visual Graph`.
- **Safe Linear Graph Editing Operations**:
  - **Adding Recorded Event Nodes**: Insert new key events, mouse events, or dedicated wait/delay nodes (`addEventNode`, `addWaitNode`). Inserts nodes directly into the linear chain between predecessor and successor nodes.
  - **Deleting Event Nodes**: Safely remove event nodes (`deleteNode`, `deleteSelectedNode`) while protecting mandatory `Start` and `End` nodes. Automatically reconnects predecessor directly to successor.
  - **Editing Event Parameters**: Modify event types, key strings, mouse buttons, coordinates, and delay durations directly in the Selected Node Inspector panel (`updateNodeEvent`, `updateNodeDelay`).
  - **Reordering Events**: Move nodes earlier or later in execution order (`moveNodeUp`, `moveNodeDown`), automatically rewiring graph links to reflect new topological order.
  - **Linear Chain Reconnection**: Cleanly repairs broken, disconnected, or missing links across the sequence with a single click (`reconnectLinearChain`).
  - **Auto Layout**: Automatically arranges nodes with uniform spacing from Start to End (`autoLayout`).
- **Snapshot-Based Undo / Redo**:
  - Maintains an internal snapshot history (`GraphEditorSnapshot`) for state changes (`undo()`, `redo()`, `canUndo()`, `canRedo()`, `pushUndoSnapshot()`), allowing non-destructive experimentation.
- **Selected Node Inspector Panel**:
  - Displays detailed properties for selected nodes (Node ID, Kind, Title, Subtitle, and Source Index).
  - Interactive parameter controls for event type combo (KeyDown, KeyUp, MouseDown, MouseUp, MouseMove, Delay/Wait), delay input, key text, mouse button selection, and coordinates.
  - Contextual action buttons for deleting or moving the selected node.
- **Validation & Safe Apply Pipeline**:
  - **Validate**: Evaluates topology and structural constraints with `ValidationOptions::sequenceGraph()`.
  - **Apply to Sequence**: Re-validates and compiles the graph back into the target `RecordedSequence`. If validation or compilation fails (e.g., cycles, branching, disconnected nodes, or missing endpoints), compilation errors are reported and the original sequence is preserved completely unchanged.

### Current Limitations

- **Linear Topology**: The current sequence execution compiler strictly enforces linear single-entry, single-exit execution (`Start -> Events -> End`). Non-linear branching or merging will be flagged during validation and rejected by the compiler.
- **No Direct Automation Execution**: The editor modifies the sequence model; automation execution continues to follow the project's existing playback service and UI trigger mechanisms.

### Usage Example

```cpp
#include "autoinput_ui/editors/sequenceGraphEditor.h"

using namespace autoinput::ui::editors;

SequenceGraphEditorState editorState;

// Add a new key down event node
autoinput::RecordedEvent keyEv{
    .type = autoinput::RecordedEventType::KeyDown,
    .delay = "20ms",
    .key = "f5"
};
graph::NodeId newId = editorState.addEventNode(keyEv);

// Undo the addition
if (editorState.canUndo())
{
    editorState.undo();
}

// In UI window render loop:
if (renderSequenceGraphEditor(selectedSequence, editorState, "SequenceGraphEditor"))
{
    // Sequence was successfully modified and compiled from graph
    markDirty();
}
```

## Configuration Graph Adapter (`ConfigGraphAdapter`)

The Configuration Graph Adapter (`autoinput_ui/graph/configGraphAdapter.h`) provides internal read-only conversion from `autoinput::ConfigData` into a `GraphDocument` representation, backing future visual configuration and dependency viewers.

### Key Capabilities & Representation

- **Global Configuration Elements**:
  - `ApplicationFilter` (`NodeKind::ApplicationFilter`): Captures the application target focus filter.
  - `BlacklistEntry` (`NodeKind::BlacklistEntry`): Represents application focus exclusion entries.
  - `GlobalEndKey` (`NodeKind::Input`): Visualizes the global emergency exit / stop hotkey.
- **Commands & Bindings**:
  - `Command` (`NodeKind::Command`): Central command node showing the command name, action, press/release timings, and preserved source index.
  - `StartKey` / `InputKey` / `InputButton` (`NodeKind::Input`): Trigger and modifier key/mouse button inputs mapped to the command.
  - `Control` (`NodeKind::Control`): Command control bindings showing action and input hotkeys with preserved source index.
  - `ExclusiveGroup` (`NodeKind::ExclusiveGroup`): Mutual exclusion group nodes with optional automatic deduplication across sharing commands.
- **Recorded Sequences**:
  - `Sequence` (`NodeKind::Sequence`): High-level sequence node showing name, event count, repeat mode, and preserved source index.
  - `SequenceStartKey` (`NodeKind::Input`): Sequence start key trigger node linked to the sequence.
- **Topological Relationships**:
  - `Input -> Command`: Directed links from start keys, input keys, and mouse buttons to their associated command.
  - `Command -> Control`: Directed links from commands to attached control actions.
  - `Command -> ExclusiveGroup`: Directed links from commands to exclusive group nodes.
  - `Sequence Start Key -> Sequence`: Directed links from sequence trigger keys to sequence nodes.
  - `Global Settings -> Targets`: Optional directed links connecting global application filters, blacklist rules, and global end keys to affected commands and sequences.
- **Deterministic 3-Column Layout**:
  - Column 0: Inputs & Global Settings (`startX`)
  - Column 1: Core Commands & Sequences (`startX + columnSpacing`)
  - Column 2: Controls & Exclusive Groups (`startX + 2 * columnSpacing`)
- **Configurable Conversion Options (`ConfigGraphOptions`)**:
  - `startX`, `startY`: Canvas placement coordinates.
  - `columnSpacing`, `rowSpacing`, `blockSpacing`: Layout spacing configuration.
  - `includeGlobalSettings`: Toggle global setting node generation.
  - `linkGlobalSettingsToTargets`: Toggle links from global settings to all commands and sequences.
  - `deduplicateExclusiveGroups`: Group shared exclusive group names into single shared nodes.

### Usage Example

```cpp
#include "autoinput_ui/graph/configGraphAdapter.h"
#include "autoinput_ui/graph/fallbackGraphViewer.h"

using namespace autoinput::ui::graph;

// Convert ConfigData to GraphDocument
ConfigGraphOptions options = ConfigGraphOptions::defaults();
options.deduplicateExclusiveGroups = true;
options.linkGlobalSettingsToTargets = true;

GraphDocument configGraph = configToGraphDocument(configData, options);

// Inspect or render via graph viewer
FallbackGraphViewerState viewerState;
renderFallbackGraphViewer(configGraph, viewerState);
```

## Read-Only Visual Configuration Graph Viewer (`ConfigGraphViewer`)

The Read-Only Visual Configuration Graph Viewer (`autoinput_ui/editors/configGraphViewer.h`) provides an interactive Dear ImGui component to visually inspect configuration topology, node attributes, and potential configuration conflicts/anomalies in `ConfigData`.

### Key Features

1. **Integrated Visual Configuration View**: Accessible directly via the `[Visual Graph]` tab in the Configuration Editor window (`ConfigEditorWindow`).
2. **Interactive Element Filtering**: Real-time toolbar toggles allowing users to filter specific graph layers:
   - Commands
   - Command Controls
   - Recorded Sequences
   - Input Triggers (Start keys, keys, mouse buttons, sequence starts, global end keys)
   - Exclusive Groups
   - Global Settings (Application filter, blacklist entries)
3. **Comprehensive Diagnostics Panel**:
   - Backed by reusable non-UI diagnostics engine (`ConfigDiagnostics`).
   - Color-coded severity indicators (Error, Warning, Info) with structured categories and suggested resolution steps.
   - Clickable "Select Node" navigation to highlight and jump to the affected element in the graph.
4. **Node Inspector Panel**:
   - Displays detailed metadata for the selected node (Kind, Title, Subtitle, Source Index).
   - Lists connected inputs, attached controls, exclusive group memberships, and targeted entities.
   - Correlates and displays diagnostics specific to the selected node.
5. **Safe Read-Only Inspection Boundary**:
   - Inspection operations are completely non-destructive and do not mutate configuration or sequence data.

## Reusable Configuration Diagnostics (`ConfigDiagnostics`)

The Reusable Configuration Diagnostics module (`autoinput_ui/graph/configDiagnostics.h`) provides headless, non-UI validation and diagnostic analysis for `autoinput::ConfigData`.

### Diagnostic Structured Fields (`ConfigDiagnosticIssue`)

Every diagnostic issue contains:
- `severity`: `ConfigDiagnosticSeverity` (`Info`, `Warning`, `Error`)
- `message`: Detailed diagnostic description
- `category`: Category string (e.g. `"Command"`, `"Control"`, `"Sequence"`, `"Input Conflict"`, `"Wildcard Control"`, `"Configuration"`)
- `commandIndex`: Optional 0-based index of the affected command in `ConfigData::commands`
- `controlIndex`: Optional 0-based index of the affected control within the parent command
- `sequenceIndex`: Optional 0-based index of the affected sequence in `ConfigData::sequences`
- `relatedInput`: Optional input trigger string (e.g. `"f1"`, `"mouse.all"`, `"space"`)
- `associatedNodeId`: Optional `NodeId` linking the issue directly to visual graph elements
- `suggestedFix`: Actionable recommendation to resolve the issue

### Diagnostic Categories & Rules

| Category | Finding | Severity | Description & Meaning |
| :--- | :--- | :--- | :--- |
| **Command** | Empty Command Name | `Warning` | Command has an empty or whitespace name; recommends assigning a descriptive name. |
| **Command** | Duplicate Command Name | `Warning` | Multiple commands define identical names; flags naming ambiguity. |
| **Command** | Empty Command Action | `Error` | Command has no action specified (`click`, `hold`, etc.); automation cannot execute without an action. |
| **Command** | Invalid Command Action | `Error` | Command action is not recognized by `ConfigMetadata`; must be corrected to a valid action. |
| **Command** | Duplicate Intra-Command Start Keys | `Warning` | A single command defines redundant identical start triggers in its `startKeys` list. |
| **Control** | Empty Control Binding | `Error` | Command control is missing both input binding and action. |
| **Control** | Empty Control Input Binding | `Error` | Command control has an action but no input trigger specified. |
| **Control** | Empty Control Action | `Warning` | Command control has an input trigger but no control action (`pause`, `cancel`, `toggle`, etc.). |
| **Control** | Invalid Control Action | `Error` | Control action does not match valid choices in `ConfigMetadata::validControlActionChoices()`. |
| **Wildcard Control** | Broad Wildcard Trigger | `Info` | Control uses a wildcard trigger (`input.all`, `mouse.all`, `keys.all`) that will intercept broad input sets. |
| **Sequence** | Empty Sequence Name | `Warning` | Sequence has an empty name; recommends assigning a descriptive identifier. |
| **Sequence** | Duplicate Sequence Name | `Warning` | Multiple sequences share the same name. |
| **Sequence** | Empty Sequence Start Trigger | `Warning` | Sequence has no start key configured, meaning it cannot be triggered via hotkey. |
| **Input Conflict** | Duplicate Command Start Keys | `Warning` | Multiple distinct commands share the same start key. |
| **Input Conflict** | Duplicate Sequence Start Keys | `Warning` | Multiple distinct sequences share the same start key. |
| **Input Conflict** | Command & Sequence Start Conflict | `Warning` | A command and a recorded sequence share the same trigger key, causing activation ambiguity. |
| **Input Conflict** | Shared Input Keys/Buttons | `Info` | Multiple commands trigger or listen for the same key/button without exclusive group isolation. |
| **Configuration** | Core Validation Errors | `Error`/`Warning` | Incorporates standard validation findings (e.g., duration formatting, invalid hotkeys, application filters). |

### Diagnostics Usage Example

```cpp
#include "autoinput_ui/graph/configDiagnostics.h"
#include "autoinput_ui/graph/configGraphAdapter.h"

using namespace autoinput::ui::graph;

// Analyze configuration diagnostics in headless mode
ConfigDiagnosticsResult result = analyzeConfigDiagnostics(configData);

if (result.hasErrors())
{
    // Handle configuration errors
}

// Optionally correlate with GraphDocument nodes for visual editor badging
GraphDocument doc = configToGraphDocument(configData);
mapDiagnosticsToGraphNodes(result, doc);
```

## Node Editor Backend Abstraction (`INodeEditorBackend`)

The Node Editor Backend Abstraction (`autoinput_ui/graph/nodeEditorBackend.h`) decouples graph document visualization and interaction logic from specific rendering libraries.

### Backend Types & Selection Hierarchy

- `NodeEditorBackendType::Fallback`: Lightweight no-op backend and default Dear ImGui fallback viewer. Guaranteed to be available in all builds without external dependencies.
- `NodeEditorBackendType::ImNodes`: Optional integration using the immediate-mode [imnodes](https://github.com/Nelarius/imnodes) library.
- `NodeEditorBackendType::ImguiNodeEditor`: Optional integration using the advanced canvas [imgui-node-editor](https://github.com/thedmd/imgui-node-editor) library.

The backend factory selects the most capable enabled backend automatically:
1. `ImguiNodeEditor` (if compiled with `AUTOINPUT_HAS_IMGUI_NODE_EDITOR`)
2. `ImNodes` (if compiled with `AUTOINPUT_HAS_IMNODES`)
3. `Fallback` (default dependency-free fallback)

#### Runtime Backend Selection API
Developers and UI window controllers can explicitly inspect, select, or reset the active backend:
```cpp
// Check if a backend is available in the current build
if (autoinput::ui::graph::isBackendAvailable(NodeEditorBackendType::ImguiNodeEditor))
{
    // Override default selection
    autoinput::ui::graph::setPreferredBackendType(NodeEditorBackendType::ImguiNodeEditor);
}

// Reset back to automatic build priority
autoinput::ui::graph::resetPreferredBackendType();

// Create preferred backend instance
std::unique_ptr<INodeEditorBackend> backend = autoinput::ui::graph::createDefaultNodeEditorBackend();
```

### `imgui-node-editor` Backend Capabilities

When `AUTOINPUT_UI_WITH_IMGUI_NODE_EDITOR` is enabled, the `imgui-node-editor` backend wraps the advanced [imgui-node-editor](https://github.com/thedmd/imgui-node-editor) canvas library behind the `INodeEditorBackend` interface:

#### Feature Capabilities Summary

- **Supported Features**:
  - `supportsCanvas`: Renders node editor canvas using `ax::NodeEditor::Begin()` / `ax::NodeEditor::End()`.
  - `supportsPositions`: Synchronizes 2D node coordinates via `ax::NodeEditor::SetNodePosition()` and `ax::NodeEditor::GetNodePosition()`.
  - `supportsLinkCreationQuery`: Detects interactive link creation between pins via `ax::NodeEditor::BeginCreate()`, `QueryNewLink()`, and `AcceptNewItem()`.
  - `supportsLinkDeletionQuery`: Detects interactive link deletion events via `ax::NodeEditor::BeginDelete()`, `QueryDeletedLink()`, and `AcceptDeletedItem()`.
  - `supportsSelectionQuery`: Queries single and multi-node/link selection via `ax::NodeEditor::GetSelectedNodes()` and `ax::NodeEditor::GetSelectedLinks()`.
  - `supportsZoom`: Smooth canvas zooming and scaling (down to 10% overview, up to 200%).
  - `supportsGroups`: Visual hierarchical grouping frames and container boundaries.
  - `supportsComments`: Visual annotation and floating comment boxes (`NodeKind::Comment`).
  - `supportsMultiSelect`: Interactive marquee box selection of nodes and links.
- **Unsupported / Missing Features**:
  - `supportsMinimap`: Minimap rendering is disabled/unsupported in this base integration.

---

### `imnodes` Backend Capabilities

When `AUTOINPUT_UI_WITH_IMNODES` is enabled, the `imnodes` backend wraps the immediate-mode [imnodes](https://github.com/Nelarius/imnodes) library behind the `INodeEditorBackend` interface:

#### Feature Capabilities Summary

- **Supported Features**:
  - `supportsCanvas`: Renders node editor canvas using `imnodes::BeginNodeEditor()` / `imnodes::EndNodeEditor()`.
  - `supportsPositions`: Synchronizes 2D node coordinates via `imnodes::SetNodeEditorSpacePos()` and `imnodes::GetNodeEditorSpacePos()`.
  - `supportsLinkCreationQuery`: Detects interactive link creation between pins via `imnodes::IsLinkCreated()`.
  - `supportsLinkDeletionQuery`: Detects interactive link deletion events via `imnodes::IsLinkDestroyed()`.
  - `supportsSelectionQuery`: Queries single and multi-node/link selection via `imnodes::GetSelectedNodes()` and `imnodes::GetSelectedLinks()`.
  - `supportsMultiSelect`: Multi-node box selection and multi-link selection.
- **Unsupported / Missing Features**:
  - `supportsZoom`: imnodes does not support smooth canvas zooming or scaling.
  - `supportsGroups`: imnodes does not support visual hierarchical subgraph containers or grouping frames.
  - `supportsComments`: imnodes does not provide visual annotation comment boxes.
  - `supportsMinimap`: imnodes minimap rendering is disabled/unsupported in this integration.

---

### Backend Evaluation Matrix: `imgui-node-editor` vs. `imnodes` vs. `Fallback`

The following evaluation matrix compares the required visual editor features across all three available backends (`imgui-node-editor`, `imnodes`, and `Fallback`):

| Feature / Capability | `imgui-node-editor` | `imnodes` | `Fallback` (Dear ImGui) | Technical Assessment & Comparison |
| :--- | :--- | :--- | :--- | :--- |
| **Node Rendering** | **Full** (Custom layout) | **Full** (Standard boxes) | **List / Canvas fallback** | `imgui-node-editor` and `imnodes` both offer full node rendering; `imgui-node-editor` supports richer styling and custom node headers. |
| **Pin Rendering** | **Advanced** (Any widget) | **Basic** (Circle pin) | **Text badges** | `imgui-node-editor` allows custom shapes, icons, and inline widgets per pin; `imnodes` uses fixed left/right circular pin points. |
| **Link Creation** | **Interactive** (`BeginCreate`) | **Interactive** (`IsLinkCreated`) | **Read-only / Manual** | Both third-party backends provide drag-and-drop link creation and wire drawing. |
| **Link Deletion** | **Interactive** (`BeginDelete`) | **Interactive** (`IsLinkDestroyed`) | **Manual button** | Both third-party backends support interactive disconnection (Del key / context clicks). |
| **Node Selection** | **Multi-select** (Marquee) | **Multi-select** (Marquee) | **Single-item click** | Both third-party backends provide rich multi-node selection and bounding box marquee selection. |
| **Node Positioning** | **2D Canvas Sync** | **2D Canvas Sync** | **Auto-layout grid** | Both third-party backends support drag movement and coordinate synchronization with `GraphDocument`. |
| **Panning** | **Smooth Canvas** | **Canvas Drag** | **Scrollbars** | Both third-party backends support canvas panning via middle-mouse or right-mouse dragging. |
| **Zoom** | **Supported** (10%–200%) | **Unsupported** | **Unsupported** | `imgui-node-editor` supports full canvas zooming, enabling clear navigation of large multi-command configuration graphs. |
| **Minimap** | **Available** (Optional) | **Experimental** | **Unsupported** | `imgui-node-editor` provides built-in minimap drawing and navigation helpers. |
| **Groups & Comments**| **Supported** (Containers) | **Unsupported** | **Unsupported** | `imgui-node-editor` can visually frame `ExclusiveGroup`s and render floating comment boxes. |
| **Keyboard Navigation**| **Built-in** (Arrow/Nav) | **Minimal** (Del only) | **Standard ImGui tab/keys** | `imgui-node-editor` provides built-in navigation and focus handling. |
| **Context Menus** | **Native hooks** | **Manual Workaround** | **ImGui popups** | `imgui-node-editor` provides `ShowNodeContextMenu` and canvas context hooks. |
| **Styling & Flow** | **Rich** (Curvatures/Flow) | **Basic** (Style stack) | **ImGui theme** | `imgui-node-editor` supports link flow animation, custom curvatures, and shadow styling. |
| **Ease of Abstraction**| **Good** | **Excellent** | **Direct ImGui** | Both libraries map cleanly to `INodeEditorBackend` without leaking third-party types. |
| **Dependency Footprint**| **Moderate** (~5 files) | **Minimal** (~2 files) | **Zero** (Built-in) | `Fallback` has zero external dependencies; `imnodes` is minimal; `imgui-node-editor` is modular. |

---

### Detailed Feature Breakdown

1. **Node Rendering & Custom Layouts**:
   - `imgui-node-editor`: Provides complete control over node interior layout, allowing custom header badges, embedded parameter controls, and collapsible property panels.
   - `imnodes`: Renders discrete nodes with dedicated title bars and content areas (`BeginNode`, `BeginNodeTitleBar`, `EndNodeTitleBar`, `EndNode`).
   - `Fallback`: Renders nodes using plain Dear ImGui child windows and tables.

2. **Pin Rendering & Socket Types**:
   - `imgui-node-editor`: Pins are declared with `BeginPin(pinId, kind)` and `EndPin()`, allowing arbitrary Dear ImGui widgets, custom icons, or colored badges inside the pin definition.
   - `imnodes`: Pins are declared with `BeginInputAttribute(pinId)` and `BeginOutputAttribute(pinId)` and render as circular connection points on node edges.

3. **Link Creation & Deletion**:
   - `imgui-node-editor`: Interactive link creation is handled via `BeginCreate()`, `QueryNewLink()`, and `AcceptNewItem()`. Link deletion is detected via `BeginDelete()`, `QueryDeletedLink()`, and `AcceptDeletedItem()`.
   - `imnodes`: Link creation is detected via `IsLinkCreated(&startPin, &endPin)` and deletion via `IsLinkDestroyed(&linkId)`.

4. **Canvas Zoom & Large Graph Navigation**:
   - **`imgui-node-editor`**: Supports smooth canvas scaling from 10% to 200%. For `ConfigGraphViewer`, this solves the primary scalability bottleneck, allowing full-overview inspection of 50+ configuration nodes and zooming into individual command clusters.
   - **`imnodes`**: Lacks canvas zooming, requiring panning across large configuration topologies.

5. **Visual Grouping & Comments**:
   - **`imgui-node-editor`**: Supports visual container boundaries (`NodeKind::ExclusiveGroup`) and floating annotation comment boxes (`NodeKind::Comment`).
   - **`imnodes`**: Does not support grouping frames.

---

### Recommendations & Architecture Strategy

1. **Default Build Baseline: Zero-Dependency Fallback**
   - The native Dear ImGui `FallbackGraphViewer` and `FallbackNodeEditorBackend` remain the default in the codebase, ensuring default builds, CI pipelines, and automated tests require zero external dependencies.

2. **Advanced Visual Editors: `imgui-node-editor`**
   - For visual graph editing and large configuration inspection, `imgui-node-editor` is the recommended backend due to native canvas zooming, group containers, and rich pin layouts.

3. **Lightweight Alternative: `imnodes`**
   - `imnodes` remains supported as a secondary, lightweight optional backend for simple linear sequence graphs.

### Build Configuration & Optional Flags

Both third-party backends are completely optional and disabled (`OFF`) by default to ensure default builds and CI remain completely dependency-free:

- `-DAUTOINPUT_UI_WITH_IMNODES=ON`: Enables imnodes backend compilation.
- `-DAUTOINPUT_UI_WITH_IMGUI_NODE_EDITOR=ON`: Enables imgui-node-editor backend compilation.

> **Important**: Build scripts and CMake do **not** fetch or vendor third-party libraries automatically. If either option is enabled, the dependency must be provided locally via:
> - Search paths: `third_party/imnodes`, `extern/imnodes`, or `-DIMNODES_DIR=/path/to/imnodes`
> - Search paths: `third_party/imgui-node-editor`, `extern/imgui-node-editor`, or `-DIMGUI_NODE_EDITOR_DIR=/path/to/imgui-node-editor`
> If an option is enabled but the dependency files are not found, CMake emits an explicit, actionable configuration error.

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
