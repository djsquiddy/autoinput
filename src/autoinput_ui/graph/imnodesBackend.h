/**
 * @file imnodesBackend.h
 * @brief Factory declaration for imnodes-based node editor backend.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_GRAPH_IMNODES_BACKEND_H
#define INCLUDE_AUTOINPUT_UI_GRAPH_IMNODES_BACKEND_H

#include "nodeEditorBackend.h"

#include <memory>

namespace autoinput::ui::graph
{
    /**
     * @brief Creates an instance of the imnodes-based node editor backend.
     * @return Unique pointer to the imnodes backend instance, or nullptr if imnodes is not compiled.
     */
    [[nodiscard]] std::unique_ptr<INodeEditorBackend> createImnodesBackend();

} // namespace autoinput::ui::graph

#endif // INCLUDE_AUTOINPUT_UI_GRAPH_IMNODES_BACKEND_H
