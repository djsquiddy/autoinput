/**
 * @file autoInput.cpp
 * @author djsquiddy
 * @date March 2026
 */
#include "autoinput/autoInput.h"
#include "autoinput/logger.h"
#include "autoinput/platform.h"
#include "autoinput/backend.h"
#include "autoinput/backendContext.h"
#include "autoinput/backendFactory.h"
#include <ranges>
#include <format>
#include <cctype>
#include <algorithm>

namespace autoinput
{
    std::unique_ptr<IPlatformBackend> g_backend = nullptr;

    bool Program::isApplicationBlacklisted() const
    {
        if (m_arguments.blacklist.empty())
        {
            return false;
        }

        const std::string activeApp = toLowerCase(platform::getActiveApplicationName());
        for (const std::string& app : m_arguments.blacklist)
        {
            if (activeApp.find(toLowerCase(app)) != std::string::npos)
            {
                return true;
            }
        }
        return false;
    }

    bool Program::processKeyEvent(KeyboardInput&& input)
    {
        input.printInfo();

        if (isApplicationBlacklisted())
        {
            return false;
        }

        if (!m_arguments.applicationName.empty())
        {
            const std::string activeApp = toLowerCase(platform::getActiveApplicationName());
            const std::string targetApp = toLowerCase(m_arguments.applicationName);
            if (activeApp.find(targetApp) == std::string::npos)
            {
                return false;
            }
        }

        if (!input.isKeyDown())
        {
            return false;
        }

        const auto [charKey, functionKey, vk, modifier] = input.getKeyState();
        bool handled = false;
        bool started = false;

        for (const KeyInfo& keyInfo : m_keyInfo)
        {
            if ((keyInfo.keyCode != INVALID_KEY && keyInfo.keyCode == charKey) ||
                (keyInfo.functionKey != INVALID_KEY && keyInfo.functionKey == functionKey) ||
                (keyInfo.virtualKey != 0 && keyInfo.virtualKey == vk))
            {
                if (keyInfo.isStartKey)
                {
                    start(keyInfo);
                    started = true;
                }
                else if (!started)
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

        if (isApplicationBlacklisted())
        {
            return false;
        }

        if (!m_arguments.applicationName.empty())
        {
            const std::string activeApp = toLowerCase(platform::getActiveApplicationName());
            const std::string targetApp = toLowerCase(m_arguments.applicationName);
            if (activeApp.find(targetApp) == std::string::npos)
            {
                return false;
            }
        }

        const auto [trigger, isDown] = input.getButtonState();

        if (trigger == MouseButton::NONE || !isDown)
        {
            return false;
        }

        bool handled = false;
        bool started = false;

        for (const KeyInfo& keyInfo : m_keyInfo)
        {
            if (keyInfo.triggerButton == trigger)
            {
                if (keyInfo.isStartKey)
                {
                    start(keyInfo);
                    started = true;
                }
                else if (!started)
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
        if (keyInfo.mouse.button != MouseButton::NONE)
        {
            MouseHandler& handler = m_mouseHandlers.at(keyInfo.mouse);
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
            if (mouseHandler.getActive())
            {
                mouseHandler.setActive(false);
                if (mouseHandler.m_autoclickerThread)
                {
                    m_zombieThreads.push_back(std::move(mouseHandler.m_autoclickerThread));
                }
            }
        }

        for (auto& keyHandler : m_keyHandlers | std::views::values)
        {
            keyHandler.release();
            if (keyHandler.getActive())
            {
                keyHandler.setActive(false);
                if (keyHandler.m_autoclickerThread)
                {
                    m_zombieThreads.push_back(std::move(keyHandler.m_autoclickerThread));
                }
            }
        }

        platform::signalEnd();
    }

    void Program::joinThreads()
    {
        for (auto& mouseHandler : m_mouseHandlers | std::views::values)
        {
            if (mouseHandler.m_autoclickerThread && mouseHandler.m_autoclickerThread->joinable())
            {
                mouseHandler.m_autoclickerThread->join();
            }
        }

        for (auto& keyHandler : m_keyHandlers | std::views::values)
        {
            if (keyHandler.m_autoclickerThread && keyHandler.m_autoclickerThread->joinable())
            {
                keyHandler.m_autoclickerThread->join();
            }
        }

        for (auto& thread : m_zombieThreads)
        {
            if (thread && thread->joinable())
            {
                thread->join();
            }
        }
        m_zombieThreads.clear();
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
            if (keyInfo.mouse.button != MouseButton::NONE)
            {
                actionStr += std::format("{} button", keyInfo.mouse.toString());
            }
            else if (!keyInfo.key.character.empty())
            {
                actionStr += std::format("{} key", keyInfo.key.toString());
            }

            Logger::info("{}, Is used as {} key\n", triggerStr, actionStr);
        }
        Logger::flush();
    }

    void Program::onFocusChanged(const std::string& activeApp)
    {
        const std::string lowerActiveApp = toLowerCase(activeApp);
        bool shouldPause = false;

        if (!m_arguments.blacklist.empty())
        {
            for (const std::string& app : m_arguments.blacklist)
            {
                if (lowerActiveApp.find(toLowerCase(app)) != std::string::npos)
                {
                    shouldPause = true;
                    break;
                }
            }
        }

        if (!m_arguments.applicationName.empty())
        {
            const std::string targetApp = toLowerCase(m_arguments.applicationName);
            if (lowerActiveApp.find(targetApp) == std::string::npos)
            {
                shouldPause = true;
            }
        }

        for (auto& handler : m_mouseHandlers | std::views::values)
        {
            handler.setPaused(shouldPause);
        }
        for (auto& handler : m_keyHandlers | std::views::values)
        {
            handler.setPaused(shouldPause);
        }

        if (shouldPause)
        {
            Logger::debug("Application lost focus or blacklisted application focused, pausing auto-pressing.\n");
        }
        else
        {
            Logger::debug("Application focused, resuming auto-pressing.\n");
        }
    }

    // ReSharper disable once CppMemberFunctionMayBeConst
    void Program::startAutoClicker(InputHandler& handler) // NOLINT(*-make-member-function-const)
    {
        if (handler.getActive())
        {
            handler.release();
            handler.setActive(false);
            if (handler.m_autoclickerThread)
            {
                m_zombieThreads.push_back(std::move(handler.m_autoclickerThread));
            }
            return;
        }

        auto delayData = m_arguments.delayData;
        handler.setActive(true);
        handler.m_autoclickerThread = std::make_unique<std::thread>([&handler, delayData]()
        {
            while (handler.getActive())
            {
                if (handler.getPaused())
                {
                    if (handler.isPressed())
                    {
                        handler.release();
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }

                handler.press();
                const auto pressWaitTime = std::chrono::milliseconds(delayData.getPressDelay());
                Logger::debug("Pressed: {}, waiting: {}", handler.getName(), pressWaitTime);
                std::this_thread::sleep_for(pressWaitTime);
                if (!handler.getActive() || handler.getPaused())
                {
                    if (handler.isPressed())
                    {
                        handler.release();
                    }
                    continue;
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
        for (auto& mouse : m_arguments.buttons)
        {
            m_mouseHandlers[mouse] = MouseHandler{mouse};
        }

        for (auto& key : m_arguments.keys)
        {
            m_keyHandlers[key] = KeyHandler{key};
        }

        auto processKeyString = [this](const std::string& keyStr, const Mouse mouse, Key targetKey, const ActionState action, const bool isStart) {
            const auto mouseTrigger = mouseButtonFromArguments(keyStr);
            KeyInfo info{
                .mouse = mouse,
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
                processKeyString(m_arguments.startKeys[i], Mouse{}, m_arguments.keys[i - buttonCount], action, true);
            }
        }

        processKeyString(m_arguments.endKey, Mouse{}, {}, ActionState::CLICK, false);
    }

    bool installHooks()
    {
        if (!BackendRegistry::getBackend())
        {
            BackendRegistry::setBackend(BackendFactory::createPlatformBackend());
        }
        
        IPlatformBackend* backend = BackendRegistry::getBackend();
        if (!backend) return false;
        return backend->installHooks();
    }

    void runListener()
    {
        IPlatformBackend* backend = BackendRegistry::getBackend();
        if (backend) backend->runListener();
    }

    void cleanup()
    {
        IPlatformBackend* backend = BackendRegistry::getBackend();
        if (backend) backend->cleanup();
    }
}
