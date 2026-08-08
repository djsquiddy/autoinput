/**
* @file  imgui_impl_raylib.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "imgui_impl_raylib.h"
#include "autoinput/logger.h"
#include <vector>
#include <cmath>
#include "imgui.h"
#include <raylib.h>
#include <rlgl.h>

namespace autoinput::ui
{
    namespace
    {
        double g_Time = 0.0;
        Texture2D g_FontTexture = { 0 };
        ImGuiKey RaylibKeyToImGuiKey(const int key)
        {
            switch (key) {
            case KEY_TAB: return ImGuiKey_Tab;
            case KEY_LEFT: return ImGuiKey_LeftArrow;
            case KEY_RIGHT: return ImGuiKey_RightArrow;
            case KEY_UP: return ImGuiKey_UpArrow;
            case KEY_DOWN: return ImGuiKey_DownArrow;
            case KEY_PAGE_UP: return ImGuiKey_PageUp;
            case KEY_PAGE_DOWN: return ImGuiKey_PageDown;
            case KEY_HOME: return ImGuiKey_Home;
            case KEY_END: return ImGuiKey_End;
            case KEY_INSERT: return ImGuiKey_Insert;
            case KEY_DELETE: return ImGuiKey_Delete;
            case KEY_BACKSPACE: return ImGuiKey_Backspace;
            case KEY_SPACE: return ImGuiKey_Space;
            case KEY_ENTER: return ImGuiKey_Enter;
            case KEY_ESCAPE: return ImGuiKey_Escape;
            case KEY_APOSTROPHE: return ImGuiKey_Apostrophe;
            case KEY_COMMA: return ImGuiKey_Comma;
            case KEY_MINUS: return ImGuiKey_Minus;
            case KEY_PERIOD: return ImGuiKey_Period;
            case KEY_SLASH: return ImGuiKey_Slash;
            case KEY_ZERO: return ImGuiKey_0;
            case KEY_ONE: return ImGuiKey_1;
            case KEY_TWO: return ImGuiKey_2;
            case KEY_THREE: return ImGuiKey_3;
            case KEY_FOUR: return ImGuiKey_4;
            case KEY_FIVE: return ImGuiKey_5;
            case KEY_SIX: return ImGuiKey_6;
            case KEY_SEVEN: return ImGuiKey_7;
            case KEY_EIGHT: return ImGuiKey_8;
            case KEY_NINE: return ImGuiKey_9;
            case KEY_SEMICOLON: return ImGuiKey_Semicolon;
            case KEY_EQUAL: return ImGuiKey_Equal;
            case KEY_A: return ImGuiKey_A;
            case KEY_B: return ImGuiKey_B;
            case KEY_C: return ImGuiKey_C;
            case KEY_D: return ImGuiKey_D;
            case KEY_E: return ImGuiKey_E;
            case KEY_F: return ImGuiKey_F;
            case KEY_G: return ImGuiKey_G;
            case KEY_H: return ImGuiKey_H;
            case KEY_I: return ImGuiKey_I;
            case KEY_J: return ImGuiKey_J;
            case KEY_K: return ImGuiKey_K;
            case KEY_L: return ImGuiKey_L;
            case KEY_M: return ImGuiKey_M;
            case KEY_N: return ImGuiKey_N;
            case KEY_O: return ImGuiKey_O;
            case KEY_P: return ImGuiKey_P;
            case KEY_Q: return ImGuiKey_Q;
            case KEY_R: return ImGuiKey_R;
            case KEY_S: return ImGuiKey_S;
            case KEY_T: return ImGuiKey_T;
            case KEY_U: return ImGuiKey_U;
            case KEY_V: return ImGuiKey_V;
            case KEY_W: return ImGuiKey_W;
            case KEY_X: return ImGuiKey_X;
            case KEY_Y: return ImGuiKey_Y;
            case KEY_Z: return ImGuiKey_Z;
            case KEY_LEFT_BRACKET: return ImGuiKey_LeftBracket;
            case KEY_BACKSLASH: return ImGuiKey_Backslash;
            case KEY_RIGHT_BRACKET: return ImGuiKey_RightBracket;
            case KEY_GRAVE: return ImGuiKey_GraveAccent;
            case KEY_CAPS_LOCK: return ImGuiKey_CapsLock;
            case KEY_SCROLL_LOCK: return ImGuiKey_ScrollLock;
            case KEY_NUM_LOCK: return ImGuiKey_NumLock;
            case KEY_PRINT_SCREEN: return ImGuiKey_PrintScreen;
            case KEY_PAUSE: return ImGuiKey_Pause;
            case KEY_F1: return ImGuiKey_F1;
            case KEY_F2: return ImGuiKey_F2;
            case KEY_F3: return ImGuiKey_F3;
            case KEY_F4: return ImGuiKey_F4;
            case KEY_F5: return ImGuiKey_F5;
            case KEY_F6: return ImGuiKey_F6;
            case KEY_F7: return ImGuiKey_F7;
            case KEY_F8: return ImGuiKey_F8;
            case KEY_F9: return ImGuiKey_F9;
            case KEY_F10: return ImGuiKey_F10;
            case KEY_F11: return ImGuiKey_F11;
            case KEY_F12: return ImGuiKey_F12;
            case KEY_LEFT_SHIFT: return ImGuiKey_LeftShift;
            case KEY_LEFT_CONTROL: return ImGuiKey_LeftCtrl;
            case KEY_LEFT_ALT: return ImGuiKey_LeftAlt;
            case KEY_LEFT_SUPER: return ImGuiKey_LeftSuper;
            case KEY_RIGHT_SHIFT: return ImGuiKey_RightShift;
            case KEY_RIGHT_CONTROL: return ImGuiKey_RightCtrl;
            case KEY_RIGHT_ALT: return ImGuiKey_RightAlt;
            case KEY_RIGHT_SUPER: return ImGuiKey_RightSuper;
            case KEY_KB_MENU: return ImGuiKey_Menu;
            case KEY_KP_0: return ImGuiKey_Keypad0;
            case KEY_KP_1: return ImGuiKey_Keypad1;
            case KEY_KP_2: return ImGuiKey_Keypad2;
            case KEY_KP_3: return ImGuiKey_Keypad3;
            case KEY_KP_4: return ImGuiKey_Keypad4;
            case KEY_KP_5: return ImGuiKey_Keypad5;
            case KEY_KP_6: return ImGuiKey_Keypad6;
            case KEY_KP_7: return ImGuiKey_Keypad7;
            case KEY_KP_8: return ImGuiKey_Keypad8;
            case KEY_KP_9: return ImGuiKey_Keypad9;
            case KEY_KP_DECIMAL: return ImGuiKey_KeypadDecimal;
            case KEY_KP_DIVIDE: return ImGuiKey_KeypadDivide;
            case KEY_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
            case KEY_KP_SUBTRACT: return ImGuiKey_KeypadSubtract;
            case KEY_KP_ADD: return ImGuiKey_KeypadAdd;
            case KEY_KP_ENTER: return ImGuiKey_KeypadEnter;
            case KEY_KP_EQUAL: return ImGuiKey_KeypadEqual;
            default: return ImGuiKey_None;
            }
        }
    }

bool ImGui_ImplRaylib_Init()
{
    ImGuiIO& io = ImGui::GetIO();
    io.BackendPlatformName = "imgui_impl_raylib_custom";
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;

    return true;
}

void ImGui_ImplRaylib_Shutdown()
{
    if (g_FontTexture.id != 0) {
        UnloadTexture(g_FontTexture);
        g_FontTexture = { 0 };
    }
}


void ImGui_ImplRaylib_NewFrame()
{
    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = ImVec2((float)GetScreenWidth(), (float)GetScreenHeight());
    
    Vector2 scale = GetWindowScaleDPI();
    io.DisplayFramebufferScale = ImVec2(scale.x, scale.y);

    double currentTime = GetTime();
    io.DeltaTime = g_Time > 0.0 ? (float)(currentTime - g_Time) : (float)(1.0f / 60.0f);
    g_Time = currentTime;

    // Mouse
    io.AddMousePosEvent((float)GetMouseX(), (float)GetMouseY());
    
    auto handleMouseButton = [&](int rayBtn, ImGuiMouseButton imguiBtn)
    {
        if (IsMouseButtonPressed(rayBtn)) io.AddMouseButtonEvent(imguiBtn, true);
        else if (IsMouseButtonReleased(rayBtn)) io.AddMouseButtonEvent(imguiBtn, false);
    };
    handleMouseButton(MOUSE_BUTTON_LEFT, ImGuiMouseButton_Left);
    handleMouseButton(MOUSE_BUTTON_RIGHT, ImGuiMouseButton_Right);
    handleMouseButton(MOUSE_BUTTON_MIDDLE, ImGuiMouseButton_Middle);
    
    Vector2 wheel = GetMouseWheelMoveV();
    io.AddMouseWheelEvent(wheel.x, wheel.y);

    // Keyboard
    for (int key = 32; key < 349; key++)
    {
        if (IsKeyPressed(key)) io.AddKeyEvent(RaylibKeyToImGuiKey(key), true);
        else if (IsKeyReleased(key)) io.AddKeyEvent(RaylibKeyToImGuiKey(key), false);
    }
    
    // Modifiers
    io.AddKeyEvent(ImGuiMod_Ctrl, IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL));
    io.AddKeyEvent(ImGuiMod_Shift, IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT));
    io.AddKeyEvent(ImGuiMod_Alt, IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT));
    io.AddKeyEvent(ImGuiMod_Super, IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER));

    // Characters
    int charPressed = GetCharPressed();
    while (charPressed > 0)
    {
        io.AddInputCharacter(charPressed);
        charPressed = GetCharPressed();
    }
    
    // Create font texture if not done
    if (g_FontTexture.id == 0)
    {
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        
        Image image = { 0 };
        image.data = pixels;
        image.width = width;
        image.height = height;
        image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        image.mipmaps = 1;
        
        g_FontTexture = LoadTextureFromImage(image);
        io.Fonts->SetTexID((ImTextureID)(intptr_t)g_FontTexture.id);
    }
}

void ImGui_ImplRaylib_RenderDrawData(ImDrawData* draw_data)
{
    if (!draw_data || !draw_data->Valid || draw_data->CmdLists.Size == 0) return;

    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();
    rlEnableScissorTest();

    for (int n = 0; n < draw_data->CmdLists.Size; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
        const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            
            ImVec2 pos = draw_data->DisplayPos;
            rlScissor((int)((pcmd->ClipRect.x - pos.x) * draw_data->FramebufferScale.x), 
                      (int)((draw_data->DisplaySize.y - (pcmd->ClipRect.w - pos.y)) * draw_data->FramebufferScale.y), 
                      (int)((pcmd->ClipRect.z - pcmd->ClipRect.x) * draw_data->FramebufferScale.x), 
                      (int)((pcmd->ClipRect.w - pcmd->ClipRect.y) * draw_data->FramebufferScale.y));

            unsigned int textureId = (unsigned int)(intptr_t)pcmd->GetTexID();

            rlBegin(RL_TRIANGLES);
            rlSetTexture(textureId);

            for (unsigned int i = 0; i < pcmd->ElemCount; i++)
            {
                ImDrawIdx idx = idx_buffer[pcmd->IdxOffset + i];
                const ImDrawVert& v = vtx_buffer[pcmd->VtxOffset + idx];

                Color c = {
                    .r = (unsigned char)(v.col & 0xFF),
                    .g = (unsigned char)((v.col >> 8) & 0xFF),
                    .b = (unsigned char)((v.col >> 16) & 0xFF),
                    .a = (unsigned char)((v.col >> 24) & 0xFF)
                };
                rlColor4ub(c.r, c.g, c.b, c.a);
                rlTexCoord2f(v.uv.x, v.uv.y);
                rlVertex2f(v.pos.x, v.pos.y);
            }
            rlEnd();
            rlDrawRenderBatchActive();
        }
    }

    rlSetTexture(0);
    rlDisableScissorTest();
    rlEnableBackfaceCulling();
}

} // namespace autoinput::ui
