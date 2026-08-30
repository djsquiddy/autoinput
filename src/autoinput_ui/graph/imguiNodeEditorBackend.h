/**
 * @file imguiNodeEditorBackend.h
 * @brief Factory declaration for imgui-node-editor-based node editor backend.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_GRAPH_IMGUI_NODE_EDITOR_BACKEND_H
#define INCLUDE_AUTOINPUT_UI_GRAPH_IMGUI_NODE_EDITOR_BACKEND_H

#include "nodeEditorBackend.h"

#include <memory>

namespace autoinput::ui::graph
{
    /**
     * @brief Creates an instance of the imgui-node-editor-based node editor backend.
     * @return Unique pointer to the imgui-node-editor backend instance, or nullptr if imgui-node-editor is not compiled.
     */
    [[nodiscard]] std::unique_ptr<INodeEditorBackend> createImguiNodeEditorBackend();

} // namespace autoinput::ui::graph

#endif // INCLUDE_AUTOINPUT_UI_GRAPH_IMGUI_NODE_EDITOR_BACKEND_H
