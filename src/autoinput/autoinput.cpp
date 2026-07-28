/**
 * @file autoinput.cpp
 * @author djsquiddy
 * @date March 2026
 */
#include "autoinput.h"
#include "logger.h"
#include "platform.h"
#include "backend.h"

namespace autoinput
{
    std::unique_ptr<PlatformBackend> g_backend = nullptr;

    bool Program::processKeyEvent(KeyboardInput&& input)
    {
        input.printInfo();

        if (!input.isKeyDown())
        {
            return false;
        }

        const auto [charKey, functionKey, vk, modifier] = input.getKeyState();
        bool handled = false;

        for (const KeyInfo& keyInfo : m_keyInfo)
        {
            if ((keyInfo.keyCode != INVALID_KEY && keyInfo.keyCode == charKey) ||
                (keyInfo.functionKey != INVALID_KEY && keyInfo.functionKey == functionKey) ||
                (keyInfo.virtualKey != 0 && keyInfo.virtualKey == vk))
            {
                if (keyInfo.isStartKey)
                {
                    start(keyInfo);
                }
                else
                {
                    end();
                }
                handled = true;
            }
        }

        return handled;
    }

    bool Program::processMouseEvent(const MouseInput& input)
    {
        input.printInfo();

        const auto [trigger, isDown] = input.getButtonState();

        if (trigger == MouseButton::NONE || !isDown)
        {
            return false;
        }

        bool handled = false;
        for (const KeyInfo& keyInfo : m_keyInfo)
        {
            if (keyInfo.triggerButton == trigger)
            {
                if (keyInfo.isStartKey)
                {
                    start(keyInfo);
                }
                else
                {
                    end();
                }
                handled = true;
            }
        }

        return handled;
    }

    void Program::start(const KeyInfo& keyInfo)
    {
        if (keyInfo.mouseButton != MouseButton::NONE)
        {
            MouseHandler& handler = m_mouseHandlers.at(keyInfo.mouseButton);
            if (keyInfo.action == ActionState::HOLD)
            {
                handler.togglePressState();
            }
            else
            {
                startAutoClicker(handler);
            }
        }
    
        if (!keyInfo.key.character.empty())
        {
            KeyHandler& handler = m_keyHandlers.at(keyInfo.key);
            if (keyInfo.action == ActionState::HOLD)
            {
                handler.togglePressState();
            }
            else
            {
                startAutoClicker(handler);
            }
        }
    }

    void Program::end()
    {
        for (auto& mouseHandler : m_mouseHandlers | std::views::values)
        {
            mouseHandler.release();
            mouseHandler.setActive(false);
        }

        for (auto& keyHandler : m_keyHandlers | std::views::values)
        {
            keyHandler.release();
            keyHandler.setActive(false);
        }

        platform::signalEnd();
    }

    void Program::printProgramInfo() const
    {
        for (const auto& keyInfo : m_keyInfo)
        {
            std::string triggerStr;
            if (keyInfo.triggerButton != MouseButton::NONE)
            {
                triggerStr = std::format("Mouse: {}", mouseButtonToString(keyInfo.triggerButton));
            }
            else if (keyInfo.keyCode != INVALID_KEY)
            {
                triggerStr = std::format("Key: {}", static_cast<char>(keyInfo.keyCode));
            }
            else if (keyInfo.functionKey != INVALID_KEY)
            {
                triggerStr = std::format("Function: F{}", keyInfo.functionKey);
            }

            std::string actionStr = keyInfo.isStartKey ? "start " : "stop ";
            if (keyInfo.mouseButton != MouseButton::NONE)
            {
                actionStr += std::format("{} button", mouseButtonToString(keyInfo.mouseButton));
            }
            else if (!keyInfo.key.character.empty())
            {
                actionStr += std::format("{} key", keyInfo.key.toString());
            }

            Logger::info("{}, Is used as {} key\n", triggerStr, actionStr);
        }
        Logger::flush();
    }

    // ReSharper disable once CppMemberFunctionMayBeConst
    void Program::startAutoClicker(InputHandler& handler) // NOLINT(*-make-member-function-const)
    {
        if (handler.getActive())
        {
            handler.release();
            handler.setActive(false);
            handler.m_autoclickerThread->join();
            return;
        }

        auto delayData = m_arguments.delayData;
        handler.setActive(true);
        handler.m_autoclickerThread = std::make_unique<std::thread>([&handler, delayData]()
        {
            while (handler.getActive())
            {
                if (!handler.getActive())
                {
                    // Make sure we didn't just disable the callback.
                    break;
                }

                handler.press();
                const auto pressWaitTime = std::chrono::milliseconds(delayData.getPressDelay());
                Logger::debug("Pressed: {}, waiting: {}", handler.getName(), pressWaitTime);
                std::this_thread::sleep_for(pressWaitTime);
                if (!handler.getActive())
                {
                    // Make sure we didn't just disable the callback.
                    break;
                }
                handler.release();
                const auto releaseWaitTime = std::chrono::milliseconds(delayData.getReleaseDelay());
                Logger::debug("Released: {}, waiting: {}", handler.getName(), releaseWaitTime);
                std::this_thread::sleep_for(std::chrono::milliseconds(releaseWaitTime));
            }
        });
    }

    void Program::init()
    {
        for (auto& button : m_arguments.buttons)
        {
            m_mouseHandlers[button] = MouseHandler{button};
        }

        for (auto& key : m_arguments.keys)
        {
            m_keyHandlers[key] = KeyHandler{key};
        }

        auto processKeyString = [this](const std::string& keyStr, MouseButton button, Key targetKey, ActionState action, bool isStart) {
            const auto mouseTrigger = mouseButtonFromArguments(keyStr);
            KeyInfo info{
                .mouseButton = button,
                .key = std::move(targetKey),
                .action = action,
                .isStartKey = isStart,
            };

            if (mouseTrigger != MouseButton::NONE)
            {
                info.triggerButton = mouseTrigger;
            }
            else
            {
                info.triggerKey = Key::fromString(keyStr);
                info.virtualKey = platform::getVirtualKey(info.triggerKey);

                if (keyStr.length() == 1)
                {
                    info.keyCode = static_cast<int32_t>(std::tolower(keyStr[0]));
                }
                else if (std::tolower(keyStr[0]) == 'f' && keyStr.length() > 1 && std::isdigit(keyStr[1]))
                {
                    info.functionKey = parseStringToInt(keyStr.substr(1));
                }
            }
            m_keyInfo.emplace_back(std::move(info));
        };

        const size_t buttonCount = m_arguments.buttons.size();
        const size_t keyCount = m_arguments.keys.size();
        const size_t actionCount = m_arguments.targetActions.size();

        for (size_t i = 0; i < m_arguments.startKeys.size(); ++i)
        {
            const auto action = i < actionCount ? m_arguments.targetActions[i] : ActionState::CLICK;
            if (i < buttonCount)
            {
                processKeyString(m_arguments.startKeys[i], m_arguments.buttons[i], {}, action, true);
            }
            else if (i < buttonCount + keyCount)
            {
                processKeyString(m_arguments.startKeys[i], MouseButton::NONE, m_arguments.keys[i - buttonCount], action, true);
            }
        }

        processKeyString(m_arguments.endKey, MouseButton::NONE, {}, ActionState::CLICK, false);
    }

    bool installHooks()
    {
#if AUTOINPUT_FAKE_HOOK
        Logger::info("Fake hook enabled, actions will be logged but not performed.\n");
        g_backend = std::make_unique<FakeBackend>();
        return g_backend->installHooks();
#else
        if (!g_backend)
        {
#ifdef _WIN32
            g_backend = createWindowsBackend();
#else
            // On Linux, we call a helper that detects the backend
            extern std::unique_ptr<PlatformBackend> detectLinuxBackend();
            g_backend = detectLinuxBackend();
#endif
        }
        
        if (!g_backend) return false;
        return g_backend->installHooks();
#endif
    }

    void runListener()
    {
        if (g_backend) g_backend->runListener();
    }

    void cleanup()
    {
        if (g_backend) g_backend->cleanup();
    }
}
