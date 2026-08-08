/**
 * @file imgui_impl_raylib.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_IMGUI_IMPL_RAYLIB_H
#define INCLUDE_AUTOINPUT_UI_IMGUI_IMPL_RAYLIB_H
#pragma once

struct ImDrawData;

namespace autoinput::ui
{
    bool ImGui_ImplRaylib_Init();
    void ImGui_ImplRaylib_Shutdown();
    void ImGui_ImplRaylib_NewFrame();
    void ImGui_ImplRaylib_RenderDrawData(ImDrawData* draw_data);
}

#endif // INCLUDE_AUTOINPUT_UI_IMGUI_IMPL_RAYLIB_H
