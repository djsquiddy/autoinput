/**
 * @file controlBindingTest.cpp
 * @brief Unit tests for flexible per-command control bindings.
 * @author djsquiddy
 * @date August 2026
 */

#include <gtest/gtest.h>
#include "autoinput/app/autoinput.h"
#include "autoinput/config/config.h"
#include "autoinput/config/configValidator.h"
#include "autoinput/platform/backend.h"
#include "autoinput/support/types.h"
#include "testUtils.h"

#ifdef _WIN32
#include <windows.h>
#include "autoinput/platform/win32/internalDataWin32.h"
#endif

namespace autoinput
{
    namespace
    {
        class ControlBindingTestBackend : public FakeBackend
        {
        public:
            void mousePress(const Mouse& mouse) override
            {
                lastPressedButton = mouse.button;
                pressCount++;
                activePressedButtons.insert(mouse.button);
            }

            void mouseRelease(const Mouse& mouse) override
            {
                lastReleasedButton = mouse.button;
                releaseCount++;
                activePressedButtons.erase(mouse.button);
            }

            void keyDown(const Key& key) override
            {
                lastDownKey = key.character;
                keyDownCount++;
            }

            void keyUp(const Key& key) override
            {
                lastUpKey = key.character;
                keyUpCount++;
            }

            MouseButton lastPressedButton = MouseButton::None;
            MouseButton lastReleasedButton = MouseButton::None;
            std::string lastDownKey;
            std::string lastUpKey;
            int pressCount = 0;
            int releaseCount = 0;
            int keyDownCount = 0;
            int keyUpCount = 0;
            std::set<MouseButton> activePressedButtons;
        };

#ifdef _WIN32
        MouseInput createWin32MouseInput(MouseData& data, WindowsMouseData& winData, MSLLHOOKSTRUCT& mouseStruct, DWORD msg, DWORD mouseData = 0, DWORD flags = 0)
        {
            mouseStruct.mouseData = mouseData;
            mouseStruct.flags = flags;
            winData.wParam = msg;
            winData.mouseStruct = &mouseStruct;
            data.internal = winData;
            return MouseInput(data);
        }

        KeyboardInput createWin32KeyInput(KeyboardData& data, WindowsKeyboardData& winData, KBDLLHOOKSTRUCT& kbdStruct, DWORD msg, DWORD vk, DWORD flags = 0)
        {
            kbdStruct.vkCode = vk;
            kbdStruct.flags = flags;
            winData.wParam = msg;
            winData.kbdStruct = &kbdStruct;
            data.internal = winData;
            return KeyboardInput(data);
        }
#endif
    }

    TEST(ControlBindingTest, ControlBinding_StartsNamedCommand)
    {
        Program program;
        program.arguments().buttons.push_back(Mouse(MouseButton::Left));
        program.arguments().targetActions.push_back(ActionState::HOLD);
        program.arguments().commandNames.push_back("left-clicker");
        program.arguments().commandControls.push_back({
            { .action = "start", .input = "f2" }
        });
        program.setBackend(std::make_unique<ControlBindingTestBackend>());
        ASSERT_TRUE(program.init());

        const auto& keyInfos = program.getKeyInfo();
        ASSERT_FALSE(keyInfos.empty());

        EXPECT_EQ(keyInfos[0].controlAction, ControlAction::Start);
        EXPECT_EQ(keyInfos[0].name, "left-clicker");

        // Apply control action to start
        EXPECT_TRUE(program.applyControlAction(keyInfos[0]));
        EXPECT_TRUE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());

        // Applying start again does not toggle it off
        EXPECT_TRUE(program.applyControlAction(keyInfos[0]));
        EXPECT_TRUE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());
    }

    TEST(ControlBindingTest, ControlBinding_CancelStopsOnlyNamedCommand)
    {
        Program program;
        // Command 1: Left mouse button
        program.arguments().buttons.push_back(Mouse(MouseButton::Left));
        program.arguments().targetActions.push_back(ActionState::HOLD);
        program.arguments().commandNames.push_back("left-clicker");
        program.arguments().commandControls.push_back({
            { .action = "toggle", .input = "f2" },
            { .action = "cancel", .input = "f4" }
        });

        // Command 2: Right mouse button
        program.arguments().buttons.push_back(Mouse(MouseButton::Right));
        program.arguments().targetActions.push_back(ActionState::HOLD);
        program.arguments().commandNames.push_back("right-clicker");
        program.arguments().commandControls.push_back({
            { .action = "toggle", .input = "f5" }
        });

        program.setBackend(std::make_unique<ControlBindingTestBackend>());
        ASSERT_TRUE(program.init());

        // Start both commands
        program.applyControlAction(ControlAction::Start, "left-clicker");
        program.applyControlAction(ControlAction::Start, "right-clicker");

        EXPECT_TRUE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());
        EXPECT_TRUE(program.getMouseHandlers().at(Mouse(MouseButton::Right)).getActive());

        // Cancel only left-clicker
        program.stopCommand("left-clicker");

        EXPECT_FALSE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());
        EXPECT_TRUE(program.getMouseHandlers().at(Mouse(MouseButton::Right)).getActive());
    }

    TEST(ControlBindingTest, ControlBinding_StopAllStopsEveryCommand)
    {
        Program program;
        program.arguments().buttons.push_back(Mouse(MouseButton::Left));
        program.arguments().targetActions.push_back(ActionState::HOLD);
        program.arguments().commandNames.push_back("left-clicker");

        program.arguments().buttons.push_back(Mouse(MouseButton::Right));
        program.arguments().targetActions.push_back(ActionState::HOLD);
        program.arguments().commandNames.push_back("right-clicker");

        program.setBackend(std::make_unique<ControlBindingTestBackend>());
        ASSERT_TRUE(program.init());

        program.applyControlAction(ControlAction::Start, "left-clicker");
        program.applyControlAction(ControlAction::Start, "right-clicker");

        EXPECT_TRUE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());
        EXPECT_TRUE(program.getMouseHandlers().at(Mouse(MouseButton::Right)).getActive());

        // Stop all
        program.stopAllCommands();

        EXPECT_FALSE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());
        EXPECT_FALSE(program.getMouseHandlers().at(Mouse(MouseButton::Right)).getActive());
    }

    TEST(ControlBindingTest, ControlBinding_ExitPreservesLegacyEndBehavior)
    {
        Program program;
        program.arguments().buttons.push_back(Mouse(MouseButton::Left));
        program.arguments().targetActions.push_back(ActionState::HOLD);
        program.arguments().commandNames.push_back("left-clicker");
        program.arguments().endKey = "f3";

        program.setBackend(std::make_unique<ControlBindingTestBackend>());
        ASSERT_TRUE(program.init());

        program.applyControlAction(ControlAction::Start, "left-clicker");
        EXPECT_TRUE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());

        // Legacy end / exitRuntime
        program.exitRuntime();
        EXPECT_FALSE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());
    }

    TEST(ControlBindingTest, ControlBinding_PauseResumeAffectsOnlyTargetCommand)
    {
        Program program;
        program.arguments().buttons.push_back(Mouse(MouseButton::Left));
        program.arguments().targetActions.push_back(ActionState::HOLD);
        program.arguments().commandNames.push_back("cmd1");

        program.arguments().buttons.push_back(Mouse(MouseButton::Right));
        program.arguments().targetActions.push_back(ActionState::HOLD);
        program.arguments().commandNames.push_back("cmd2");

        program.setBackend(std::make_unique<ControlBindingTestBackend>());
        ASSERT_TRUE(program.init());

        program.applyControlAction(ControlAction::Start, "cmd1");
        program.applyControlAction(ControlAction::Start, "cmd2");

        EXPECT_FALSE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getPaused());
        EXPECT_FALSE(program.getMouseHandlers().at(Mouse(MouseButton::Right)).getPaused());

        // Pause only cmd1
        program.pauseCommand("cmd1");
        EXPECT_TRUE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getPaused());
        EXPECT_FALSE(program.getMouseHandlers().at(Mouse(MouseButton::Right)).getPaused());

        // Resume cmd1
        program.resumeCommand("cmd1");
        EXPECT_FALSE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getPaused());
        EXPECT_FALSE(program.getMouseHandlers().at(Mouse(MouseButton::Right)).getPaused());
    }

    TEST(ControlBindingTest, ControlBinding_TogglePauseTogglesTargetCommand)
    {
        Program program;
        program.arguments().buttons.push_back(Mouse(MouseButton::Left));
        program.arguments().targetActions.push_back(ActionState::HOLD);
        program.arguments().commandNames.push_back("cmd1");

        program.setBackend(std::make_unique<ControlBindingTestBackend>());
        ASSERT_TRUE(program.init());

        program.applyControlAction(ControlAction::Start, "cmd1");
        EXPECT_FALSE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getPaused());

        program.togglePauseCommand("cmd1");
        EXPECT_TRUE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getPaused());

        program.togglePauseCommand("cmd1");
        EXPECT_FALSE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getPaused());
    }

    TEST(ControlBindingTest, ControlBinding_ExclusiveGroupBehavior)
    {
        Program program;
        program.arguments().buttons.push_back(Mouse(MouseButton::Left));
        program.arguments().targetActions.push_back(ActionState::HOLD);
        program.arguments().commandNames.push_back("cmd1");
        program.arguments().exclusiveGroups.push_back("groupA");
        program.arguments().commandControls.push_back({ { .action = "start", .input = "f1" } });

        program.arguments().buttons.push_back(Mouse(MouseButton::Right));
        program.arguments().targetActions.push_back(ActionState::HOLD);
        program.arguments().commandNames.push_back("cmd2");
        program.arguments().exclusiveGroups.push_back("groupA");
        program.arguments().commandControls.push_back({ { .action = "start", .input = "f2" } });

        program.setBackend(std::make_unique<ControlBindingTestBackend>());
        ASSERT_TRUE(program.init());

        // Start cmd1
        EXPECT_TRUE(program.applyControlAction(program.getKeyInfo()[0]));
        EXPECT_TRUE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());
        EXPECT_FALSE(program.getMouseHandlers().at(Mouse(MouseButton::Right)).getActive());

        // Start cmd2 -> cmd1 should be stopped because of exclusiveGroup
        EXPECT_TRUE(program.applyControlAction(program.getKeyInfo()[1]));
        EXPECT_FALSE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());
        EXPECT_TRUE(program.getMouseHandlers().at(Mouse(MouseButton::Right)).getActive());
    }

    TEST(ControlBindingTest, ControlBinding_BlacklistPreventsStartingAndResuming)
    {
        Program program;
        program.arguments().buttons.push_back(Mouse(MouseButton::Left));
        program.arguments().targetActions.push_back(ActionState::HOLD);
        program.arguments().commandNames.push_back("cmd1");
        program.arguments().blacklist.push_back("notepad.exe");
        program.arguments().commandControls.push_back({ { .action = "start", .input = "f1" } });

        program.setBackend(std::make_unique<ControlBindingTestBackend>());
        ASSERT_TRUE(program.init());

        program.setTestActiveApp("notepad.exe");
        EXPECT_TRUE(program.isApplicationBlacklisted());

        // Attempt start
        EXPECT_FALSE(program.applyControlAction(program.getKeyInfo()[0]));
        EXPECT_FALSE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());

        // Clear blacklist condition and start
        program.setTestActiveApp("calculator.exe");
        EXPECT_FALSE(program.isApplicationBlacklisted());
        EXPECT_TRUE(program.applyControlAction(program.getKeyInfo()[0]));
        EXPECT_TRUE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());

        // Pause command
        program.pauseCommand("cmd1");
        EXPECT_TRUE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getPaused());

        // Set blacklisted again and try resume -> resume should be blocked
        program.setTestActiveApp("notepad.exe");
        program.resumeCommand("cmd1");
        EXPECT_TRUE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getPaused());
    }

    TEST(ControlBindingTest, ControlBinding_InvalidActionFailsValidation)
    {
        CommandData cmd;
        cmd.name = "test";
        cmd.action = "click";
        cmd.buttons = { "left" };
        cmd.controls = {
            { .action = "invalid_action_name", .input = "f2" }
        };

        const auto errors = validateCommandData(cmd);
        EXPECT_FALSE(errors.empty());
        bool foundControlActionError = false;
        for (const auto& err : errors)
        {
            if (err.field == "action" && err.section.find("controls") != std::string::npos)
            {
                foundControlActionError = true;
            }
        }
        EXPECT_TRUE(foundControlActionError);
    }

    TEST(ControlBindingTest, ControlBinding_InvalidInputFailsValidation)
    {
        CommandData cmd;
        cmd.name = "test";
        cmd.action = "click";
        cmd.buttons = { "left" };
        cmd.controls = {
            { .action = "toggle", .input = "invalid_trigger_name_xyz" }
        };

        const auto errors = validateCommandData(cmd);
        EXPECT_FALSE(errors.empty());
        bool foundControlInputError = false;
        for (const auto& err : errors)
        {
            if (err.field == "input" && err.section.find("controls") != std::string::npos)
            {
                foundControlInputError = true;
            }
        }
        EXPECT_TRUE(foundControlInputError);
    }

    TEST(ControlBindingTest, ControlBinding_MouseBackStartMouseRightCancel)
    {
        Program program;
        program.arguments().buttons.push_back(Mouse(MouseButton::Left));
        program.arguments().targetActions.push_back(ActionState::CLICK);
        program.arguments().commandNames.push_back("left-clicker");
        program.arguments().commandControls.push_back({
            { .action = "toggle", .input = "mouse.back" },
            { .action = "cancel", .input = "mouse.right" }
        });

        program.setBackend(std::make_unique<ControlBindingTestBackend>());
        ASSERT_TRUE(program.init());

        const auto& keyInfos = program.getKeyInfo();
        ASSERT_GE(keyInfos.size(), 2);

        // First control: Mouse Back toggle
        const KeyInfo* backControl = nullptr;
        const KeyInfo* rightControl = nullptr;
        for (const auto& info : keyInfos)
        {
            if (info.triggerButton == MouseButton::Back && info.controlAction == ControlAction::Toggle)
            {
                backControl = &info;
            }
            if (info.triggerButton == MouseButton::Right && info.controlAction == ControlAction::Cancel)
            {
                rightControl = &info;
            }
        }

        ASSERT_NE(backControl, nullptr);
        ASSERT_NE(rightControl, nullptr);

        // 1. Mouse Back starts left-clicker
        EXPECT_TRUE(program.applyControlAction(*backControl));
        EXPECT_TRUE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());

        // 2. Mouse Right cancels left-clicker
        EXPECT_TRUE(program.applyControlAction(*rightControl));
        EXPECT_FALSE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());
    }

    TEST(ControlBindingTest, LegacyStartEndBehavior_RemainsCompatible)
    {
        Program program;
        program.arguments().buttons.push_back(Mouse(MouseButton::Left));
        program.arguments().targetActions.push_back(ActionState::HOLD);
        program.arguments().startKeys.push_back("f2");
        program.arguments().endKey = "f3";

        program.setBackend(std::make_unique<ControlBindingTestBackend>());
        ASSERT_TRUE(program.init());

        const auto& keyInfos = program.getKeyInfo();
        ASSERT_EQ(keyInfos.size(), 2);

        // KeyInfo 0: Start key (F2) -> Toggle
        EXPECT_EQ(keyInfos[0].controlAction, ControlAction::Toggle);
        EXPECT_TRUE(keyInfos[0].isStartKey);

        // KeyInfo 1: End key (F3) -> Exit
        EXPECT_EQ(keyInfos[1].controlAction, ControlAction::Exit);
        EXPECT_FALSE(keyInfos[1].isStartKey);

        // Start toggles on
        program.start(keyInfos[0]);
        EXPECT_TRUE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());

        // Start toggles off
        program.start(keyInfos[0]);
        EXPECT_FALSE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());
    }

    TEST(ControlBindingTest, ControlBinding_TomlSerialization)
    {
        ConfigData configData;
        CommandData cmd;
        cmd.name = "left-clicker";
        cmd.action = "click";
        cmd.buttons = { "left" };
        cmd.controls = {
            { .action = "toggle", .input = "mouse.back" },
            { .action = "cancel", .input = "mouse.right" }
        };
        configData.commands.push_back(cmd);
        configData.endKey = "f3";

        test::TemporaryDirectory tempDir("ctrl_toml_test");
        std::filesystem::path configPath = tempDir.path() / "test_ctrl.toml";

        ASSERT_TRUE(saveConfigData(configData, configPath));

        auto loadedOpt = loadConfigData(configPath);
        ASSERT_TRUE(loadedOpt.has_value());

        const auto& loaded = *loadedOpt;
        ASSERT_EQ(loaded.commands.size(), 1);
        EXPECT_EQ(loaded.commands[0].name, "left-clicker");
        ASSERT_EQ(loaded.commands[0].controls.size(), 2);
        EXPECT_EQ(loaded.commands[0].controls[0].action, "toggle");
        EXPECT_EQ(loaded.commands[0].controls[0].input, "mouse.back");
        EXPECT_EQ(loaded.commands[0].controls[1].action, "cancel");
        EXPECT_EQ(loaded.commands[0].controls[1].input, "mouse.right");
    }

#ifdef _WIN32
    TEST(ControlBindingTest, ControlBinding_IgnoresSyntheticInput)
    {
        Program program;
        program.arguments().buttons.push_back(Mouse(MouseButton::Left));
        program.arguments().targetActions.push_back(ActionState::HOLD);
        program.arguments().commandControls.push_back({
            { .action = "start", .input = "f2" }
        });
        program.setBackend(std::make_unique<ControlBindingTestBackend>());
        ASSERT_TRUE(program.init());

        // Synthetic key press (LLKHF_INJECTED)
        KeyboardData keyData;
        WindowsKeyboardData winKeyData;
        KBDLLHOOKSTRUCT kbdStruct{};
        KeyboardInput syntheticKey = createWin32KeyInput(keyData, winKeyData, kbdStruct, WM_KEYDOWN, VK_F2, LLKHF_INJECTED);
        EXPECT_TRUE(syntheticKey.isSynthetic());
        EXPECT_FALSE(program.processKeyEvent(std::move(syntheticKey)));
        EXPECT_FALSE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());

        // Synthetic mouse press (LLMHF_INJECTED)
        MouseData mouseData;
        WindowsMouseData winMouseData;
        MSLLHOOKSTRUCT mouseStruct{};
        MouseInput syntheticMouse = createWin32MouseInput(mouseData, winMouseData, mouseStruct, WM_RBUTTONDOWN, 0, LLMHF_INJECTED);
        EXPECT_TRUE(syntheticMouse.isSynthetic());
        EXPECT_FALSE(program.processMouseEvent(syntheticMouse));
    }

    TEST(ControlBindingTest, ControlBinding_WildcardInputsPassValidation)
    {
        EXPECT_TRUE(isValidTrigger("mouse.all"));
        EXPECT_TRUE(isValidTrigger("mouse.*"));
        EXPECT_TRUE(isValidTrigger("mouse.any"));
        EXPECT_TRUE(isValidTrigger("mouse_all"));
        EXPECT_TRUE(isValidTrigger("keys.all"));
        EXPECT_TRUE(isValidTrigger("key.all"));
        EXPECT_TRUE(isValidTrigger("keys.*"));
        EXPECT_TRUE(isValidTrigger("input.all"));
        EXPECT_TRUE(isValidTrigger("input.*"));
        EXPECT_TRUE(isValidTrigger("input.any"));
        EXPECT_TRUE(isValidTrigger("all"));
        EXPECT_TRUE(isValidTrigger("any"));

        CommandData cmd;
        cmd.name = "wildcard-command";
        cmd.action = "hold";
        cmd.buttons = { "left" };
        cmd.controls = {
            { .action = "toggle", .input = "mouse.back" },
            { .action = "cancel", .input = "mouse.all" },
            { .action = "pause", .input = "keys.all" },
            { .action = "stop-all", .input = "input.all" }
        };

        const auto errors = validateCommandData(cmd);
        EXPECT_TRUE(errors.empty());
    }

    TEST(ControlBindingTest, ControlBinding_MouseAllCancelsCommandOnAnyMouseButton)
    {
        Program program;
        program.arguments().buttons.push_back(Mouse(MouseButton::Left));
        program.arguments().targetActions.push_back(ActionState::HOLD);
        program.arguments().commandNames.push_back("left-hold");
        program.arguments().commandControls.push_back({
            { .action = "start", .input = "f2" },
            { .action = "cancel", .input = "mouse.all" }
        });
        program.setBackend(std::make_unique<ControlBindingTestBackend>());
        ASSERT_TRUE(program.init());

        // Start command via F2 key event
        KeyboardData keyData;
        WindowsKeyboardData winKeyData;
        KBDLLHOOKSTRUCT kbdStruct{};
        KeyboardInput startKey = createWin32KeyInput(keyData, winKeyData, kbdStruct, WM_KEYDOWN, VK_F2, 0);
        EXPECT_TRUE(program.processKeyEvent(std::move(startKey)));
        EXPECT_TRUE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());

        // Keyboard press (e.g. Space) should NOT trigger mouse.all cancel
        KeyboardData spaceData;
        WindowsKeyboardData winSpaceData;
        KBDLLHOOKSTRUCT spaceStruct{};
        KeyboardInput spaceKey = createWin32KeyInput(spaceData, winSpaceData, spaceStruct, WM_KEYDOWN, VK_SPACE, 0);
        EXPECT_FALSE(program.processKeyEvent(std::move(spaceKey)));
        EXPECT_TRUE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());

        // Mouse Right click should trigger mouse.all cancel without swallowing input
        MouseData mouseData;
        WindowsMouseData winMouseData;
        MSLLHOOKSTRUCT mouseStruct{};
        MouseInput rightClick = createWin32MouseInput(mouseData, winMouseData, mouseStruct, WM_RBUTTONDOWN, 0, 0);
        EXPECT_FALSE(program.processMouseEvent(rightClick));
        EXPECT_FALSE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());
    }

    TEST(ControlBindingTest, ControlBinding_KeysAllCancelsCommandOnAnyKey)
    {
        Program program;
        program.arguments().buttons.push_back(Mouse(MouseButton::Left));
        program.arguments().targetActions.push_back(ActionState::HOLD);
        program.arguments().commandNames.push_back("left-hold");
        program.arguments().commandControls.push_back({
            { .action = "start", .input = "mouse.back" },
            { .action = "cancel", .input = "keys.all" }
        });
        program.setBackend(std::make_unique<ControlBindingTestBackend>());
        ASSERT_TRUE(program.init());

        // Start command via Mouse Back
        MouseData startMouseData;
        WindowsMouseData winStartMouseData;
        MSLLHOOKSTRUCT startMouseStruct{};
        MouseInput backClick = createWin32MouseInput(startMouseData, winStartMouseData, startMouseStruct, WM_XBUTTONDOWN, XBUTTON1 << 16, 0);
        EXPECT_TRUE(program.processMouseEvent(backClick));
        EXPECT_TRUE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());

        // Mouse Right click should NOT trigger keys.all cancel
        MouseData rightData;
        WindowsMouseData winRightData;
        MSLLHOOKSTRUCT rightStruct{};
        MouseInput rightClick = createWin32MouseInput(rightData, winRightData, rightStruct, WM_RBUTTONDOWN, 0, 0);
        EXPECT_FALSE(program.processMouseEvent(rightClick));
        EXPECT_TRUE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());

        // Any keyboard key (e.g. Space) should trigger keys.all cancel without swallowing input
        KeyboardData keyData;
        WindowsKeyboardData winKeyData;
        KBDLLHOOKSTRUCT kbdStruct{};
        KeyboardInput anyKey = createWin32KeyInput(keyData, winKeyData, kbdStruct, WM_KEYDOWN, VK_SPACE, 0);
        EXPECT_FALSE(program.processKeyEvent(std::move(anyKey)));
        EXPECT_FALSE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());
    }

    TEST(ControlBindingTest, ControlBinding_InputAllCancelsCommandOnMouseOrKey)
    {
        Program program;
        program.arguments().buttons.push_back(Mouse(MouseButton::Left));
        program.arguments().targetActions.push_back(ActionState::HOLD);
        program.arguments().commandNames.push_back("left-hold");
        program.arguments().commandControls.push_back({
            { .action = "start", .input = "f2" },
            { .action = "cancel", .input = "input.all" }
        });
        program.setBackend(std::make_unique<ControlBindingTestBackend>());
        ASSERT_TRUE(program.init());

        // 1. Start and cancel via mouse without swallowing mouse click
        KeyboardData keyData1;
        WindowsKeyboardData winKeyData1;
        KBDLLHOOKSTRUCT kbdStruct1{};
        KeyboardInput startKey1 = createWin32KeyInput(keyData1, winKeyData1, kbdStruct1, WM_KEYDOWN, VK_F2, 0);
        EXPECT_TRUE(program.processKeyEvent(std::move(startKey1)));
        EXPECT_TRUE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());

        MouseData mouseData;
        WindowsMouseData winMouseData;
        MSLLHOOKSTRUCT mouseStruct{};
        MouseInput mouseClick = createWin32MouseInput(mouseData, winMouseData, mouseStruct, WM_MBUTTONDOWN, 0, 0);
        EXPECT_FALSE(program.processMouseEvent(mouseClick));
        EXPECT_FALSE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());

        // Release F2 from pressed keys set so it can be pressed again
        KeyboardData keyReleaseData;
        WindowsKeyboardData winKeyReleaseData;
        KBDLLHOOKSTRUCT kbdReleaseStruct{};
        KeyboardInput releaseKey = createWin32KeyInput(keyReleaseData, winKeyReleaseData, kbdReleaseStruct, WM_KEYUP, VK_F2, 0);
        program.processKeyEvent(std::move(releaseKey));

        // 2. Start and cancel via keyboard without swallowing keystroke
        KeyboardData keyData2;
        WindowsKeyboardData winKeyData2;
        KBDLLHOOKSTRUCT kbdStruct2{};
        KeyboardInput startKey2 = createWin32KeyInput(keyData2, winKeyData2, kbdStruct2, WM_KEYDOWN, VK_F2, 0);
        EXPECT_TRUE(program.processKeyEvent(std::move(startKey2)));
        EXPECT_TRUE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());

        KeyboardData anyKeyData;
        WindowsKeyboardData winAnyKeyData;
        KBDLLHOOKSTRUCT anyKeyStruct{};
        KeyboardInput anyKey = createWin32KeyInput(anyKeyData, winAnyKeyData, anyKeyStruct, WM_KEYDOWN, VK_ESCAPE, 0);
        EXPECT_FALSE(program.processKeyEvent(std::move(anyKey)));
        EXPECT_FALSE(program.getMouseHandlers().at(Mouse(MouseButton::Left)).getActive());
    }

    TEST(ControlBindingTest, ControlBinding_MultipleCommandsSameButtonWithWildcardCancel)
    {
        // Replicate pitt.toml scenario:
        // Command 1: left-hold (hold left mouse, toggle mouse.forward, cancel mouse.all, exclusiveGroup left-click-mode)
        // Command 2: left-click (click left mouse, toggle mouse.back, cancel mouse.right, exclusiveGroup left-click-mode)
        Program program;
        program.arguments().buttons.push_back(Mouse(MouseButton::Left));
        program.arguments().targetActions.push_back(ActionState::HOLD);
        program.arguments().commandNames.push_back("left-hold");
        program.arguments().exclusiveGroups.push_back("left-click-mode");
        program.arguments().commandControls.push_back({
            { .action = "toggle", .input = "mouse.forward" },
            { .action = "cancel", .input = "mouse.all" }
        });

        program.arguments().buttons.push_back(Mouse(MouseButton::Left));
        program.arguments().targetActions.push_back(ActionState::CLICK);
        program.arguments().commandNames.push_back("left-click");
        program.arguments().exclusiveGroups.push_back("left-click-mode");
        program.arguments().commandControls.push_back({
            { .action = "toggle", .input = "mouse.back" },
            { .action = "cancel", .input = "mouse.right" }
        });

        auto testBackend = std::make_unique<ControlBindingTestBackend>();
        auto* backendPtr = testBackend.get();
        program.setBackend(std::move(testBackend));
        ASSERT_TRUE(program.init());

        // Press mouse.forward to start left-hold
        MouseData forwardData;
        WindowsMouseData winForwardData;
        MSLLHOOKSTRUCT forwardStruct{};
        MouseInput forwardInput = createWin32MouseInput(forwardData, winForwardData, forwardStruct, WM_XBUTTONDOWN, XBUTTON2 << 16, 0);
        EXPECT_TRUE(program.processMouseEvent(forwardInput));
        EXPECT_TRUE(backendPtr->activePressedButtons.contains(MouseButton::Left));

        // Click right mouse: should trigger mouse.all cancel on left-hold, release left button, and NOT consume the right click
        MouseData rightData1;
        WindowsMouseData winRightData1;
        MSLLHOOKSTRUCT rightStruct1{};
        MouseInput rightClick1 = createWin32MouseInput(rightData1, winRightData1, rightStruct1, WM_RBUTTONDOWN, 0, 0);
        EXPECT_FALSE(program.processMouseEvent(rightClick1));
        EXPECT_FALSE(backendPtr->activePressedButtons.contains(MouseButton::Left));

        // Press mouse.forward again to start left-hold
        EXPECT_TRUE(program.processMouseEvent(forwardInput));
        EXPECT_TRUE(backendPtr->activePressedButtons.contains(MouseButton::Left));

        // Click left mouse: should trigger mouse.all cancel on left-hold, release left button, and NOT consume the click
        MouseData leftData;
        WindowsMouseData winLeftData;
        MSLLHOOKSTRUCT leftStruct{};
        MouseInput leftClick = createWin32MouseInput(leftData, winLeftData, leftStruct, WM_LBUTTONDOWN, 0, 0);
        EXPECT_FALSE(program.processMouseEvent(leftClick));
        EXPECT_FALSE(backendPtr->activePressedButtons.contains(MouseButton::Left));

        // Subsequent left clicks should not be consumed and left button remains unpressed
        EXPECT_FALSE(program.processMouseEvent(leftClick));
        EXPECT_FALSE(backendPtr->activePressedButtons.contains(MouseButton::Left));

        // Press mouse.back to start left-click
        MouseData backData;
        WindowsMouseData winBackData;
        MSLLHOOKSTRUCT backStruct{};
        MouseInput backInput = createWin32MouseInput(backData, winBackData, backStruct, WM_XBUTTONDOWN, XBUTTON1 << 16, 0);
        EXPECT_TRUE(program.processMouseEvent(backInput));

        // Right click cancels left-click without consuming right click
        MouseData rightData;
        WindowsMouseData winRightData;
        MSLLHOOKSTRUCT rightStruct{};
        MouseInput rightClick = createWin32MouseInput(rightData, winRightData, rightStruct, WM_RBUTTONDOWN, 0, 0);
        EXPECT_FALSE(program.processMouseEvent(rightClick));
    }
#endif
}
