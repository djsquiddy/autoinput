/**
 * @file imguiScope.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_CORE_IMGUI_SCOPE_H
#define INCLUDE_AUTOINPUT_UI_CORE_IMGUI_SCOPE_H
#pragma once

#include <imgui.h>

namespace autoinput::ui
{
    /**
     * @brief RAII helper for ImGui::PushID and ImGui::PopID.
     * 
     * @example
     * @code
     * {
     *     ImGuiIdScope idScope("my_unique_id");
     *     if (ImGui::Button("Click Me")) { ... }
     * } // ID is automatically popped here
     * @endcode
     */
    struct ImGuiIdScope
    {
        /**
         * @brief Pushes a string ID.
         */
        explicit ImGuiIdScope(const char* id) { ImGui::PushID(id); }

        /**
         * @brief Pushes an integer ID.
         */
        explicit ImGuiIdScope(int id) { ImGui::PushID(id); }

        /**
         * @brief Pushes an size_t ID.
         */
        explicit ImGuiIdScope(size_t id) { ImGui::PushID(static_cast<int>(id)); }

        /**
         * @brief Pushes a pointer ID.
         */
        explicit ImGuiIdScope(const void* id) { ImGui::PushID(id); }

        /**
         * @brief Pops the ID.
         */
        ~ImGuiIdScope() { ImGui::PopID(); }

        ImGuiIdScope(const ImGuiIdScope&) = delete;
        ImGuiIdScope& operator=(const ImGuiIdScope&) = delete;
    };

    /**
     * @brief RAII helper for ImGui::TreeNode and ImGui::TreePop.
     * 
     * @example
     * @code
     * if (auto treeScope = ImGuiTreeNodeScope("Advanced Settings"))
     * {
     *     ImGui::Checkbox("Enable feature", &featureEnabled);
     * } // TreePop() is automatically called if TreeNode() returned true
     * @endcode
     */
    struct ImGuiTreeNodeScope
    {
        bool isOpen;

        /**
         * @brief Creates a tree node.
         * @param label The label of the node.
         */
        explicit ImGuiTreeNodeScope(const char* label) : isOpen{ ImGui::TreeNode(label) } {}

        /**
         * @brief Pops the tree node if it was open.
         */
        ~ImGuiTreeNodeScope() { if (isOpen) ImGui::TreePop(); }

        /**
         * @brief Checks if the node is open.
         */
        explicit operator bool() const { return isOpen; }

        ImGuiTreeNodeScope(const ImGuiTreeNodeScope&) = delete;
        ImGuiTreeNodeScope& operator=(const ImGuiTreeNodeScope&) = delete;
    };

    struct ImGuiFontScope
    {
        explicit ImGuiFontScope()
        {
            ImGui::PushFont(ImGui::GetFont());
        }
        ~ImGuiFontScope()
        {
            ImGui::PopFont();
        }
    };

    struct ImGuiFontSizeScope
    {
        explicit ImGuiFontSizeScope(const float newScale)
        {
            ImGui::SetWindowFontScale(newScale);
        }
        ~ImGuiFontSizeScope()
        {
            ImGui::SetWindowFontScale(1.0f);
        }
    };
}

#endif // INCLUDE_AUTOINPUT_UI_CORE_IMGUI_SCOPE_H
