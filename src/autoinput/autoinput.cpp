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

        const auto [charKey, functionKey, vk, modifier] = input.getKeyState();
        bool handled = false;

        if (m_recorder)
        {
            const Key currentKey = { input.getChar() != 0 ? std::string(1, input.getChar()) : "", modifier };
            const Key recordStartKey = Key::fromString(m_arguments.recordStartKey);
            const Key recordEndKey = Key::fromString(m_arguments.recordEndKey);

            // We match by virtual key or character if possible
            auto keysMatch = [&](const Key& k1, const Key& k2) {
                if (k1.character == k2.character && k1.modifier == k2.modifier) return true;
                if (platform::getVirtualKey(k1) == platform::getVirtualKey(k2)) return true;
                return false;
            };

            if (m_recorder->getState() == SequenceRecorder::State::Waiting)
            {
                if (input.isKeyDown() && keysMatch(recordStartKey, { "", modifier }))
                {
                    // Check if it's a function key match
                    if (modifier == KeyModifier::Function && functionKey == parseStringToInt(m_arguments.recordStartKey.substr(1)))
                    {
                        m_recorder->start();
                        return true;
                    }
                }
                // Also check for character match if not a function key
                if (input.isKeyDown() && !m_arguments.recordStartKey.empty() && m_arguments.recordStartKey[0] != 'f')
                {
                     if (std::tolower(input.getChar()) == std::tolower(m_arguments.recordStartKey[0]))
                     {
                         m_recorder->start();
                         return true;
                     }
                }
            }
            else if (m_recorder->getState() == SequenceRecorder::State::Recording)
            {
                if (input.isKeyDown() && keysMatch(recordEndKey, { "", modifier }))
                {
                    if (modifier == KeyModifier::Function && functionKey == parseStringToInt(m_arguments.recordEndKey.substr(1)))
                    {
                        m_recorder->stop();
                        
                        // Save recording
                        RecordedSequence seq = m_recorder->getSequence();
                        seq.start = m_arguments.recordPlayStartKey;
                        
                        ConfigData configData = m_arguments.toConfigData();
                        configData.sequences.push_back(std::move(seq));
                        
                        const auto savePath = getUserConfigsPath() / (m_arguments.recordName + ".toml");
                        if (saveConfigData(configData, savePath))
                        {
                            Logger::info("Recorded sequence saved to {}\n", savePath.string());
                        }
                        else
                        {
                            Logger::error("Failed to save recorded sequence to {}\n", savePath.string());
                        }

                        platform::signalEnd();
                        return true;
                    }
                }

                // Record the event
                Key k;
                if (modifier == KeyModifier::Function) k.character = "f" + std::to_string(functionKey);
                else if (input.getChar() != 0) k.character = std::string(1, input.getChar());
                k.modifier = modifier;

                m_recorder->recordKeyEvent(k, input.isKeyDown());
                return true;
            }
            return false;
        }

        if (!input.isKeyDown())
        {
            return false;
        }

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

        if (m_recorder && m_recorder->getState() == SequenceRecorder::State::Recording)
        {
            const auto [button, isDown] = input.getButtonState();
            const auto [x, y] = m_backend->getCursorPosition();

            if (button != MouseButton::None)
            {
                m_recorder->recordMouseEvent(Mouse{ button }, isDown, x, y);
            }
            else if (m_arguments.recordMouseMoves)
            {
                auto now = std::chrono::steady_clock::now();
                auto sampleDelay = parseWaitDelay(m_arguments.recordMouseSample);
                if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastMouseSampleTime) >= sampleDelay)
                {
                    m_recorder->recordMouseMove(x, y);
                    m_lastMouseSampleTime = now;
                }
            }
            return true;
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
        }
        updateStatusIndicator();
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

    void Program::updateStatusIndicator()
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

        if (isActive != m_lastIsActiveIndicator)
        {
            if (m_notificationService)
            {
                m_notificationService->notifyStatus(isActive);
            }
            else
            {
                // Fallback for cases where m_notificationService is not yet initialized (e.g. tests)
                if (isActive)
                {
                    terminal::printStatus("Auto clicking: ", "ACTIVE", terminal::Color::Green);
                }
                else
                {
                    terminal::printStatus("Auto clicking: ", "PAUSED", terminal::Color::Yellow);
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
