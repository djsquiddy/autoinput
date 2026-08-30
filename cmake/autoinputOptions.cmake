# Rename to AUTOINPUT_ENABLE_KEYBOARD_HOOK
option(ENABLE_KEYBOARD_HOOK "Enable the keyboard listeners" ON)
# Rename to AUTOINPUT_ENABLE_MOUSE_HOOK
option(ENABLE_MOUSE_HOOK "Enable the mouse listeners" ON)
# Rename to AUTOINPUT_ENABLE_FAKE_HOOK
option(ENABLE_FAKE_HOOK "Enable only logging the events instead of actually performing them" OFF)

option(AUTOINPUT_BUILD_TESTS "Build autoinput tests" OFF)
option(AUTOINPUT_BUILD_TRAY "Build the system tray" OFF)
option(AUTOINPUT_BUILD_UI "Build the UI" OFF)
option(AUTOINPUT_UI_WITH_IMNODES "Enable optional imnodes backend for UI node editors" OFF)
option(AUTOINPUT_UI_WITH_IMGUI_NODE_EDITOR "Enable optional imgui-node-editor backend for UI node editors" OFF)
option(AUTOINPUT_ENABLE_CLANG_TIDY "Run clang-tidy during builds" OFF)
