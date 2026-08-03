/**
 * @file autoInput.cpp
 * @author djsquiddy
 * @date March 2026
 */
#include "autoinput/autoInput.h"
#include "autoinput/logger.h"
#include "autoinput/platform.h"
#include "autoinput/backend.h"
#include "autoinput/backendFactory.h"
#include "autoinput/terminal.h"
#include <ranges>
#include <format>
#include <cctype>
#include <algorithm>

namespace autoinput
{
    Program::Program(std::unique_ptr<IPlatformBackend> backend)
        : m_backend(std::move(backend))
    {
    }


    void Program::setBackend(std::unique_ptr<IPlatformBackend> backend)
    {
        m_backend = std::move(backend);
    }

    bool Program::installHooks()
    {
        if (m_backend)
        {
            return m_backend->installHooks();
        }
        return false;
    }

    void Program::runListener()
    {
        if (m_backend)
        {
            m_backend->runListener();
        }
    }

    void Program::cleanup()
    {
        if (m_backend)
        {
            m_backend->cleanup();
        }
    }

    bool Program::isApplicationBlacklisted() const
    {
        if (m_arguments.blacklist.empty())
        {
            return false;
        }

        std::string activeApp;
#ifdef AUTOINPUT_TESTING
        if (!m_testActiveApp.empty())
        {
            activeApp = toLowerCase(m_testActiveApp);
        }
        else
        {
            activeApp = toLowerCase(platform::getActiveApplicationName());
        }
#else
        activeApp = toLowerCase(platform::getActiveApplicationName());
#endif

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

        if (m_recorder)
        {
            const auto keyState = input.getKeyState();
            const Key key = Key::fromKeyState(keyState);
            m_recorder->onKeyEvent(key, input.isKeyDown(), input.isSynthetic());
            if (m_recorder->getState() == RecorderState::Finished)
            {
                m_recorder->save(m_arguments.saveConfigName, m_arguments.forceOverwrite);
                platform::signalEnd();
            }
            return true; // During recording we handle everything.
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
                    if (isApplicationBlacklisted())
                    {
                        continue;
                    }

                    if (!m_arguments.applicationName.empty())
                    {
                        std::string activeApp;
#ifdef AUTOINPUT_TESTING
                        if (!m_testActiveApp.empty())
                        {
                            activeApp = toLowerCase(m_testActiveApp);
                        }
                        else
                        {
                            activeApp = toLowerCase(platform::getActiveApplicationName());
                        }
#else
                        activeApp = toLowerCase(platform::getActiveApplicationName());
#endif
                        const std::string targetApp = toLowerCase(m_arguments.applicationName);
                        if (activeApp.find(targetApp) == std::string::npos)
                        {
                            continue;
                        }
                    }

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

        if (m_recorder)
        {
            if (input.isMouseMove())
            {
                const auto [x, y] = m_backend->getCursorPosition();
                m_recorder->onMouseMove(x, y, input.isSynthetic());
            }
            else
            {
                const auto [trigger, isDown] = input.getButtonState();
                const auto [x, y] = m_backend->getCursorPosition();
                m_recorder->onMouseEvent(Mouse(trigger), isDown, x, y, input.isSynthetic());
            }
            
            if (m_recorder->getState() == RecorderState::Finished)
            {
                m_recorder->save(m_arguments.saveConfigName, m_arguments.forceOverwrite);
                platform::signalEnd();
            }
            return true; // During recording we handle everything.
        }

        const auto [trigger, isDown] = input.getButtonState();

        if (trigger == MouseButton::None || !isDown)
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
                    if (isApplicationBlacklisted())
                    {
                        continue;
                    }

                    if (!m_arguments.applicationName.empty())
                    {
                        std::string activeApp;
#ifdef AUTOINPUT_TESTING
                        if (!m_testActiveApp.empty())
                        {
                            activeApp = toLowerCase(m_testActiveApp);
                        }
                        else
                        {
                            activeApp = toLowerCase(platform::getActiveApplicationName());
                        }
#else
                        activeApp = toLowerCase(platform::getActiveApplicationName());
#endif
                        const std::string targetApp = toLowerCase(m_arguments.applicationName);
                        if (activeApp.find(targetApp) == std::string::npos)
                        {
                            continue;
                        }
                    }

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
        InputHandler* handlerToStart = nullptr;
        if (keyInfo.mouse.button != MouseButton::None)
        {
            if (m_mouseHandlers.contains(keyInfo.mouse))
            {
                handlerToStart = &m_mouseHandlers.at(keyInfo.mouse);
            }
        }
        else if (!keyInfo.key.character.empty())
        {
            if (m_keyHandlers.contains(keyInfo.key))
            {
                handlerToStart = &m_keyHandlers.at(keyInfo.key);
            }
            else if (m_sequenceHandlers.contains(keyInfo.key))
            {
                handlerToStart = &m_sequenceHandlers.at(keyInfo.key);
            }
        }
        else if (keyInfo.triggerKey.character.empty() && keyInfo.triggerButton == MouseButton::None)
        {
            // This might be a sequence trigger matched by name or from a config key that didn't populate triggerKey correctly
            // But usually we match by trigger.
        }

        if (handlerToStart)
        {
            if (!keyInfo.exclusiveGroup.empty() && !handlerToStart->getActive())
            {
                for (auto& mouseHandler : m_mouseHandlers | std::views::values)
                {
                    if (mouseHandler.getActive() && mouseHandler.getExclusiveGroup() == keyInfo.exclusiveGroup)
                    {
                        mouseHandler.release();
                        mouseHandler.setActive(false);
                        if (mouseHandler.m_autoclickerThread)
                        {
                            m_zombieThreads.push_back(std::move(mouseHandler.m_autoclickerThread));
                        }
                    }
                }
                for (auto& keyHandler : m_keyHandlers | std::views::values)
                {
                    if (keyHandler.getActive() && keyHandler.getExclusiveGroup() == keyInfo.exclusiveGroup)
                    {
                        keyHandler.release();
                        keyHandler.setActive(false);
                        if (keyHandler.m_autoclickerThread)
                        {
                            m_zombieThreads.push_back(std::move(keyHandler.m_autoclickerThread));
                        }
                    }
                }
                for (auto& seqHandler : m_sequenceHandlers | std::views::values)
                {
                    if (seqHandler.getActive() && seqHandler.getExclusiveGroup() == keyInfo.exclusiveGroup)
                    {
                        seqHandler.release();
                        seqHandler.setActive(false);
                        if (seqHandler.m_autoclickerThread)
                        {
                            m_zombieThreads.push_back(std::move(seqHandler.m_autoclickerThread));
                        }
                    }
                }
            }

            if (keyInfo.action == ActionState::HOLD)
            {
                handlerToStart->togglePressState();
            }
            else
            {
                startAutoClicker(*handlerToStart);
            }
            updateStatusIndicator(keyInfo.name, handlerToStart->getActive());
        }
        else
        {
            updateStatusIndicator(keyInfo.name);
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

        for (auto& seqHandler : m_sequenceHandlers | std::views::values)
        {
            seqHandler.release();
            if (seqHandler.getActive())
            {
                seqHandler.setActive(false);
                if (seqHandler.m_autoclickerThread)
                {
                    m_zombieThreads.push_back(std::move(seqHandler.m_autoclickerThread));
                }
            }
        }

        platform::signalEnd();
        updateStatusIndicator();
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

        for (auto& seqHandler : m_sequenceHandlers | std::views::values)
        {
            if (seqHandler.m_autoclickerThread && seqHandler.m_autoclickerThread->joinable())
            {
                seqHandler.m_autoclickerThread->join();
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
            if (keyInfo.triggerButton != MouseButton::None)
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
            if (keyInfo.mouse.button != MouseButton::None)
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
        updateStatusIndicator();
    }

    void Program::updateStatusIndicator(const std::string& triggeredCommandName, std::optional<bool> triggeredCommandActive)
    {
        if (m_arguments.jsonOutput)
        {
            return;
        }

        bool isActive = false;
        for (const auto& handler : m_mouseHandlers | std::views::values)
        {
            if (handler.getActive() && !handler.getPaused())
            {
                isActive = true;
                break;
            }
        }
        if (!isActive)
        {
            for (const auto& handler : m_keyHandlers | std::views::values)
            {
                if (handler.getActive() && !handler.getPaused())
                {
                    isActive = true;
                    break;
                }
            }
        }
        if (!isActive)
        {
            for (const auto& handler : m_sequenceHandlers | std::views::values)
            {
                if (handler.getActive() && !handler.getPaused())
                {
                    isActive = true;
                    break;
                }
            }
        }

        if (isActive != m_lastIsActiveIndicator || !triggeredCommandName.empty())
        {
            if (m_notificationService)
            {
                m_notificationService->notifyStatus(isActive, triggeredCommandName, triggeredCommandActive);
            }
            else
            {
                // Fallback for cases where m_notificationService is not yet initialized (e.g. tests)
                const bool displayActive = triggeredCommandActive.value_or(isActive);
                if (displayActive)
                {
                    if (triggeredCommandName.empty())
                    {
                        terminal::printStatus("Auto clicking: ", "ACTIVE", terminal::Color::Green);
                    }
                    else
                    {
                        terminal::printStatus(std::format("Auto clicking ({}): ", triggeredCommandName), "ACTIVE", terminal::Color::Green);
                    }
                }
                else
                {
                    if (triggeredCommandName.empty())
                    {
                        terminal::printStatus("Auto clicking: ", "PAUSED", terminal::Color::Yellow);
                    }
                    else
                    {
                        terminal::printStatus(std::format("Auto clicking ({}): ", triggeredCommandName), "PAUSED", terminal::Color::Yellow);
                    }
                }
            }
            m_lastIsActiveIndicator = isActive;
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
                Logger::debug("Pressed: {}, waiting: {}\n", handler.getName(), pressWaitTime);
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
                Logger::debug("Released: {}, waiting: {}\n", handler.getName(), releaseWaitTime);
                std::this_thread::sleep_for(std::chrono::milliseconds(releaseWaitTime));
            }
        });
    }

    bool Program::init()
    {
        if (!m_backend)
        {
            Logger::error("Program::init() called without a backend!\n");
            return false;
        }

        IPlatformBackend* backendPtr = m_backend.get();
        const size_t buttonCount = m_arguments.buttons.size();
        for (size_t i = 0; i < buttonCount; ++i)
        {
            auto& mouse = m_arguments.buttons[i];
            m_mouseHandlers[mouse] = MouseHandler{mouse, backendPtr};
            if (i < m_arguments.commandNames.size())
            {
                m_mouseHandlers[mouse].setName(m_arguments.commandNames[i]);
            }
            if (i < m_arguments.exclusiveGroups.size())
            {
                m_mouseHandlers[mouse].setExclusiveGroup(m_arguments.exclusiveGroups[i]);
            }
        }

        const size_t keyCount = m_arguments.keys.size();
        for (size_t i = 0; i < keyCount; ++i)
        {
            auto& key = m_arguments.keys[i];
            m_keyHandlers[key] = KeyHandler{key, backendPtr};
            if (i + buttonCount < m_arguments.commandNames.size())
            {
                m_keyHandlers[key].setName(m_arguments.commandNames[i + buttonCount]);
            }
            if (i + buttonCount < m_arguments.exclusiveGroups.size())
            {
                m_keyHandlers[key].setExclusiveGroup(m_arguments.exclusiveGroups[i + buttonCount]);
            }
        }

        // Initialize sequences from arguments (e.g., from loaded config)
        for (const auto& sequenceData : m_arguments.sequences)
        {
            Key startKey = Key::fromString(sequenceData.start);
            m_sequenceHandlers[startKey] = SequenceHandler{ sequenceData, backendPtr };
        }

        auto processKeyString = [this](const std::string& keyStr, const Mouse mouse, Key targetKey, const ActionState action, const bool isStart, const std::string& name = "", const std::string& group = "") {
            const auto mouseTrigger = mouseButtonFromArguments(keyStr);
            KeyInfo info{
                .mouse = mouse,
                .key = std::move(targetKey),
                .action = action,
                .isStartKey = isStart,
                .name = name,
                .exclusiveGroup = group,
            };

            if (mouseTrigger != MouseButton::None)
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

        for (const auto& sequence : m_arguments.sequences)
        {
            processKeyString(sequence.start, {}, {}, ActionState::CLICK, true, sequence.name);
        }

        const size_t actionCount = m_arguments.targetActions.size();
        const size_t nameCount = m_arguments.commandNames.size();
        const size_t groupCount = m_arguments.exclusiveGroups.size();

        for (size_t i = 0; i < m_arguments.startKeys.size(); ++i)
        {
            const auto action = i < actionCount ? m_arguments.targetActions[i] : ActionState::CLICK;
            const std::string& name = i < nameCount ? m_arguments.commandNames[i] : "";
            const std::string& group = i < groupCount ? m_arguments.exclusiveGroups[i] : "";

            if (i < buttonCount)
            {
                processKeyString(m_arguments.startKeys[i], m_arguments.buttons[i], {}, action, true, name, group);
            }
            else if (i < buttonCount + keyCount)
            {
                processKeyString(m_arguments.startKeys[i], Mouse{}, m_arguments.keys[i - buttonCount], action, true, name, group);
            }
        }

        if (m_arguments.endKey.empty())
        {
            processKeyString("f3", Mouse{}, {}, ActionState::CLICK, false);
        }
        else
        {
            processKeyString(m_arguments.endKey, Mouse{}, {}, ActionState::CLICK, false);
        }

        if (!m_arguments.recordName.empty())
        {
            m_recorder = std::make_unique<SequenceRecorder>(
                m_arguments.recordName,
                m_arguments.recordStartKey,
                m_arguments.recordEndKey,
                m_arguments.recordPlayStartKey,
                m_arguments.recordMouseMoves,
                m_arguments.recordMouseSample
            );
            Logger::info("Recording mode active. Press {} to start recording, {} to stop.\n", m_arguments.recordStartKey, m_arguments.recordEndKey);
        }

        m_notificationService = std::make_unique<NotificationService>(m_arguments.statusNotificationMode, m_arguments.jsonOutput);

        return true;
    }

    bool installHooks()
    {
        if (g_program)
        {
            return g_program->installHooks();
        }
        return false;
    }

    void runListener()
    {
        if (g_program)
        {
            g_program->runListener();
        }
    }

    void cleanup()
    {
        if (g_program)
        {
            g_program->cleanup();
        }
    }
}
