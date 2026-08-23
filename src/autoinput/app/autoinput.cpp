/**
 * @file autoInput.cpp
 * @author djsquiddy
 * @date March 2026
 */
#include "autoinput/app/autoinput.h"
#include "autoinput/config/configMetadata.h"
#include "autoinput/support/logger.h"
#include "autoinput/platform/platform.h"
#include "autoinput/platform/backend.h"
#include "autoinput/platform/terminal.h"
#include <ranges>
#include <format>
#include <cctype>
#include <algorithm>

namespace autoinput
{
    Program::Program(std::unique_ptr<IPlatformBackend> backend)
        : m_backend{ std::move(backend) }
    {
    }


    void Program::setBackend(std::unique_ptr<IPlatformBackend> backend)
    {
        m_backend = std::move(backend);
    }

    bool Program::init()
    {
        if (!m_backend)
        {
            Logger::error("Program::init() called without a backend!\n");
            return false;
        }

        IPlatformBackend* backendPtr = m_backend.get();
        m_mouseHandlers.clear();
        m_keyHandlers.clear();
        m_sequenceHandlers.clear();
        m_additionalHandlers.clear();

        const size_t buttonCount = m_arguments.buttons.size();
        for (size_t i = 0; i < buttonCount; ++i)
        {
            auto& mouse = m_arguments.buttons[i];
            const std::string name = i < m_arguments.commandNames.size() ? m_arguments.commandNames[i] : "";
            const std::string group = i < m_arguments.exclusiveGroups.size() ? m_arguments.exclusiveGroups[i] : "";

            if (!m_mouseHandlers.contains(mouse))
            {
                m_mouseHandlers[mouse] = MouseHandler{mouse, backendPtr};
                if (!name.empty())
                {
                    m_mouseHandlers[mouse].setName(name);
                }
                if (!group.empty())
                {
                    m_mouseHandlers[mouse].setExclusiveGroup(group);
                }
            }
            else
            {
                auto extra = std::make_unique<MouseHandler>(mouse, backendPtr);
                if (!name.empty())
                {
                    extra->setName(name);
                }
                if (!group.empty())
                {
                    extra->setExclusiveGroup(group);
                }
                m_additionalHandlers.push_back(std::move(extra));
            }
        }

        const size_t keyCount = m_arguments.keys.size();
        for (size_t i = 0; i < keyCount; ++i)
        {
            auto& key = m_arguments.keys[i];
            const std::string name = i + buttonCount < m_arguments.commandNames.size() ? m_arguments.commandNames[i + buttonCount] : "";
            const std::string group = i + buttonCount < m_arguments.exclusiveGroups.size() ? m_arguments.exclusiveGroups[i + buttonCount] : "";

            if (!m_keyHandlers.contains(key))
            {
                m_keyHandlers[key] = KeyHandler{key, backendPtr};
                if (!name.empty())
                {
                    m_keyHandlers[key].setName(name);
                }
                if (!group.empty())
                {
                    m_keyHandlers[key].setExclusiveGroup(group);
                }
            }
            else
            {
                auto extra = std::make_unique<KeyHandler>(key, backendPtr);
                if (!name.empty())
                {
                    extra->setName(name);
                }
                if (!group.empty())
                {
                    extra->setExclusiveGroup(group);
                }
                m_additionalHandlers.push_back(std::move(extra));
            }
        }

        // Initialize sequences from arguments (e.g., from loaded config)
        for (const auto& sequenceData : m_arguments.sequences)
        {
            Key startKey = Key::fromString(sequenceData.start);
            if (!m_sequenceHandlers.contains(startKey))
            {
                m_sequenceHandlers[startKey] = SequenceHandler{ sequenceData, backendPtr };
            }
            else
            {
                m_additionalHandlers.push_back(std::make_unique<SequenceHandler>(sequenceData, backendPtr));
            }
        }

        bool success = true;
        auto processKeyString = [this, &success](const std::string& keyStr, const Mouse mouse, Key targetKey, const ActionState action, const bool isStart, const ControlAction controlAction = ControlAction::Toggle, const std::string& name = "", const std::string& group = "") {
            const auto mouseTrigger = mouseButtonFromArguments(keyStr);
            KeyInfo info{
                .mouse = mouse,
                .key = std::move(targetKey),
                .action = action,
                .isStartKey = isStart,
                .controlAction = controlAction,
                .name = name,
                .exclusiveGroup = group,
            };

            if (ConfigMetadata::isInputAllTrigger(keyStr))
            {
                info.matchAnyInput = true;
            }
            else if (ConfigMetadata::isMouseAllTrigger(keyStr))
            {
                info.matchAnyMouse = true;
                info.triggerButton = MouseButton::All;
            }
            else if (ConfigMetadata::isKeysAllTrigger(keyStr))
            {
                info.matchAnyKey = true;
            }
            else if (mouseTrigger != MouseButton::None)
            {
                info.triggerButton = mouseTrigger;
                if (mouseTrigger == MouseButton::All)
                {
                    info.matchAnyMouse = true;
                }
            }
            else
            {
                info.triggerKey = Key::fromString(keyStr);
                info.virtualKey = platform::getVirtualKey(info.triggerKey);

                if (info.virtualKey == 0 && !info.triggerKey.character.empty())
                {
                    Logger::error("Invalid key: {}\n", keyStr);
                    success = false;
                }

                if (keyStr.length() == 1)
                {
                    info.keyCode = std::tolower(keyStr[0]);
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
            processKeyString(sequence.start, {}, {}, ActionState::CLICK, true, ControlAction::Toggle, sequence.name);
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
                processKeyString(m_arguments.startKeys[i], m_arguments.buttons[i], {}, action, true, ControlAction::Toggle, name, group);
            }
            else if (i < buttonCount + keyCount)
            {
                processKeyString(m_arguments.startKeys[i], Mouse{}, m_arguments.keys[i - buttonCount], action, true, ControlAction::Toggle, name, group);
            }
        }

        for (size_t i = 0; i < m_arguments.commandControls.size(); ++i)
        {
            const auto& controls = m_arguments.commandControls[i];
            const auto action = i < actionCount ? m_arguments.targetActions[i] : ActionState::CLICK;
            const std::string& name = i < nameCount ? m_arguments.commandNames[i] : "";
            const std::string& group = i < groupCount ? m_arguments.exclusiveGroups[i] : "";
            const auto targetMouse = i < buttonCount ? m_arguments.buttons[i] : Mouse{};
            const auto targetKey = (i >= buttonCount && i < buttonCount + keyCount) ? m_arguments.keys[i - buttonCount] : Key{};

            for (const auto& ctrl : controls)
            {
                const ControlAction ctrlAction = controlActionFromString(ctrl.action);
                if (ctrlAction != ControlAction::Invalid && !ctrl.input.empty())
                {
                    const bool isStart = (ctrlAction == ControlAction::Start || ctrlAction == ControlAction::Toggle);
                    processKeyString(ctrl.input, targetMouse, targetKey, action, isStart, ctrlAction, name, group);
                }
            }
        }

        if (m_arguments.endKey.empty())
        {
            processKeyString("f3", Mouse{}, {}, ActionState::CLICK, false, ControlAction::Exit);
        }
        else
        {
            processKeyString(m_arguments.endKey, Mouse{}, {}, ActionState::CLICK, false, ControlAction::Exit);
        }

        if (!m_arguments.recordName.empty())
        {
            m_recorder = std::make_unique<SequenceRecorder>(
                SequenceConfig{
                    .recordMouseMoves = m_arguments.recordMouseMoves,
                    .name = m_arguments.recordName,
                    .startKey = m_arguments.recordStartKey,
                    .endKey = m_arguments.recordEndKey,
                    .playStartKey = m_arguments.recordPlayStartKey,
                    .mouseSampleDelay = m_arguments.recordMouseSample
                }
            );
            Logger::info("Recording mode active. Press {} to start recording, {} to stop.\n", m_arguments.recordStartKey, m_arguments.recordEndKey);
        }

        m_notificationService = std::make_unique<NotificationService>(m_arguments.statusNotificationMode, m_arguments.jsonOutput);

        return success;
    }

    // ReSharper disable once CppMemberFunctionMayBeConst
    bool Program::installHooks()
    {
        if (m_backend)
        {
            return m_backend->installHooks();
        }
        return false;
    }

    // ReSharper disable once CppMemberFunctionMayBeConst
    void Program::runListener()
    {
        if (m_backend)
        {
            m_backend->runListener();
        }
    }

    void Program::requestStop()
    {
        if (m_backend)
        {
            m_backend->requestStop();
        }
    }

    void Program::cleanup()
    {
        if (m_backend)
        {
            m_backend->cleanup();
        }
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

        if (input.isSynthetic())
        {
            return false;
        }

        const auto [charKey, functionKey, vk, modifier] = input.getKeyState();
        if (input.isKeyUp())
        {
            m_keysPressed.erase(vk);
            return false;
        }

        if (input.isKeyDown())
        {
            if (m_keysPressed.contains(vk))
            {
                return false;
            }
            m_keysPressed.insert(vk);
        }
        else
        {
            return false;
        }
        bool handled = false;
        std::vector<const KeyInfo*> exactMatches;

        auto isExactKeyMatch = [&](const KeyInfo& keyInfo) {
            if (keyInfo.matchAnyInput || keyInfo.matchAnyKey)
            {
                return false;
            }
            return (keyInfo.keyCode != INVALID_KEY && keyInfo.keyCode == charKey) ||
                   (keyInfo.functionKey != INVALID_KEY && keyInfo.functionKey == functionKey) ||
                   (keyInfo.virtualKey != 0 && keyInfo.virtualKey == vk);
        };

        auto isSameTarget = [](const KeyInfo& a, const KeyInfo& b) {
            if (!a.name.empty() || !b.name.empty())
            {
                return a.name == b.name;
            }
            if (a.mouse.button != MouseButton::None && b.mouse.button != MouseButton::None && a.mouse == b.mouse)
            {
                return true;
            }
            if (!a.key.character.empty() && !b.key.character.empty() && a.key == b.key)
            {
                return true;
            }
            return false;
        };

        // Pass 1: Execute exact key matches
        for (const KeyInfo& keyInfo : m_keyInfo)
        {
            if (isExactKeyMatch(keyInfo))
            {
                if (applyControlAction(keyInfo))
                {
                    if (keyInfo.controlAction != ControlAction::Stop && keyInfo.controlAction != ControlAction::Cancel)
                    {
                        handled = true;
                    }
                }
                exactMatches.push_back(&keyInfo);
            }
        }

        // Pass 2: Execute wildcard matches (skip if target was exact-matched in the same event)
        for (const KeyInfo& keyInfo : m_keyInfo)
        {
            if (keyInfo.matchAnyInput || keyInfo.matchAnyKey)
            {
                const bool targetAlreadyHandled = std::ranges::any_of(exactMatches, [&](const KeyInfo* exact) {
                    return isSameTarget(*exact, keyInfo);
                });

                if (!targetAlreadyHandled)
                {
                    applyControlAction(keyInfo);
                }
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

        if (trigger == MouseButton::None || !isDown || input.isSynthetic())
        {
            return false;
        }

        bool handled = false;
        std::vector<const KeyInfo*> exactMatches;

        auto isExactMouseMatch = [&](const KeyInfo& keyInfo) {
            if (keyInfo.matchAnyInput || keyInfo.matchAnyMouse || keyInfo.triggerButton == MouseButton::All)
            {
                return false;
            }
            return keyInfo.triggerButton == trigger;
        };

        auto isSameTarget = [](const KeyInfo& a, const KeyInfo& b) {
            if (!a.name.empty() || !b.name.empty())
            {
                return a.name == b.name;
            }
            if (a.mouse.button != MouseButton::None && b.mouse.button != MouseButton::None && a.mouse == b.mouse)
            {
                return true;
            }
            if (!a.key.character.empty() && !b.key.character.empty() && a.key == b.key)
            {
                return true;
            }
            return false;
        };

        // Pass 1: Execute exact mouse button matches
        for (const KeyInfo& keyInfo : m_keyInfo)
        {
            if (isExactMouseMatch(keyInfo))
            {
                if (applyControlAction(keyInfo))
                {
                    if (keyInfo.controlAction != ControlAction::Stop && keyInfo.controlAction != ControlAction::Cancel)
                    {
                        handled = true;
                    }
                }
                exactMatches.push_back(&keyInfo);
            }
        }

        // Pass 2: Execute wildcard matches (skip if target was exact-matched in the same event)
        for (const KeyInfo& keyInfo : m_keyInfo)
        {
            if (keyInfo.matchAnyInput || keyInfo.matchAnyMouse ||
                (keyInfo.triggerButton != MouseButton::None && static_cast<bool>(keyInfo.triggerButton & trigger)))
            {
                if (isExactMouseMatch(keyInfo))
                {
                    continue;
                }

                const bool targetAlreadyHandled = std::ranges::any_of(exactMatches, [&](const KeyInfo* exact) {
                    return isSameTarget(*exact, keyInfo);
                });

                if (!targetAlreadyHandled)
                {
                    applyControlAction(keyInfo);
                }
            }
        }

        return handled;
    }

    bool Program::isTargetApplicationActive() const
    {
        if (m_arguments.applicationName.empty())
        {
            return true;
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
        const std::string targetApp = toLowerCase(m_arguments.applicationName);
        return activeApp.find(targetApp) != std::string::npos;
    }

    std::vector<InputHandler*> Program::getAllHandlers()
    {
        std::vector<InputHandler*> handlers;
        for (auto& [mouse, handler] : m_mouseHandlers)
        {
            handlers.push_back(&handler);
        }
        for (auto& [key, handler] : m_keyHandlers)
        {
            handlers.push_back(&handler);
        }
        for (auto& [key, handler] : m_sequenceHandlers)
        {
            handlers.push_back(&handler);
        }
        for (auto& handlerPtr : m_additionalHandlers)
        {
            if (handlerPtr)
            {
                handlers.push_back(handlerPtr.get());
            }
        }
        return handlers;
    }

    std::vector<const InputHandler*> Program::getAllHandlers() const
    {
        std::vector<const InputHandler*> handlers;
        for (const auto& [mouse, handler] : m_mouseHandlers)
        {
            handlers.push_back(&handler);
        }
        for (const auto& [key, handler] : m_keyHandlers)
        {
            handlers.push_back(&handler);
        }
        for (const auto& [key, handler] : m_sequenceHandlers)
        {
            handlers.push_back(&handler);
        }
        for (const auto& handlerPtr : m_additionalHandlers)
        {
            if (handlerPtr)
            {
                handlers.push_back(handlerPtr.get());
            }
        }
        return handlers;
    }

    InputHandler* Program::findHandlerByName(const std::string_view name)
    {
        if (name.empty()) return nullptr;
        for (InputHandler* handler : getAllHandlers())
        {
            if (handler && handler->getName() == name) return handler;
        }
        return nullptr;
    }

    InputHandler* Program::getHandlerForKeyInfo(const KeyInfo& keyInfo)
    {
        if (!keyInfo.name.empty())
        {
            if (InputHandler* handler = findHandlerByName(keyInfo.name))
            {
                return handler;
            }
        }
        if (keyInfo.mouse.button != MouseButton::None)
        {
            if (m_mouseHandlers.contains(keyInfo.mouse))
            {
                return &m_mouseHandlers.at(keyInfo.mouse);
            }
        }
        if (!keyInfo.key.character.empty())
        {
            if (m_keyHandlers.contains(keyInfo.key))
            {
                return &m_keyHandlers.at(keyInfo.key);
            }
            if (m_sequenceHandlers.contains(keyInfo.key))
            {
                return &m_sequenceHandlers.at(keyInfo.key);
            }
        }
        return nullptr;
    }

    void Program::stopExclusiveGroup(const std::string& group)
    {
        if (group.empty()) return;
        for (InputHandler* handler : getAllHandlers())
        {
            if (handler && handler->getActive() && handler->getExclusiveGroup() == group)
            {
                stopHandler(*handler);
            }
        }
    }

    void Program::stopHandler(InputHandler& handler)
    {
        const bool wasActiveOrPaused = handler.getActive() || handler.getPaused();
        handler.release();
        if (wasActiveOrPaused)
        {
            handler.setActive(false);
            handler.setPaused(false);
            handler.m_autoclickerThread.request_stop();
            handler.m_cv.notify_all();
            updateStatusIndicator(handler.getName(), false);
        }
    }

    void Program::pauseHandler(InputHandler& handler)
    {
        handler.setPaused(true);
        if (handler.isPressed())
        {
            handler.release();
        }
        handler.m_cv.notify_all();
        updateStatusIndicator(handler.getName(), handler.getActive());
    }

    void Program::resumeHandler(InputHandler& handler)
    {
        if (isApplicationBlacklisted() || !isTargetApplicationActive())
        {
            return;
        }
        handler.setPaused(false);
        handler.m_cv.notify_all();
        updateStatusIndicator(handler.getName(), handler.getActive());
    }

    void Program::togglePauseHandler(InputHandler& handler)
    {
        if (handler.getPaused())
        {
            resumeHandler(handler);
        }
        else
        {
            pauseHandler(handler);
        }
    }

    void Program::start(const KeyInfo& keyInfo)
    {
        applyControlAction(keyInfo);
    }

    bool Program::applyControlAction(const KeyInfo& keyInfo)
    {
        switch (keyInfo.controlAction)
        {
        case ControlAction::Start:
        {
            if (isApplicationBlacklisted() || !isTargetApplicationActive())
            {
                return false;
            }
            InputHandler* handler = getHandlerForKeyInfo(keyInfo);
            if (handler)
            {
                if (!keyInfo.exclusiveGroup.empty() && !handler->getActive())
                {
                    stopExclusiveGroup(keyInfo.exclusiveGroup);
                }
                if (!handler->getActive())
                {
                    if (keyInfo.action == ActionState::HOLD)
                    {
                        handler->press();
                        handler->setActive(true);
                    }
                    else
                    {
                        startAutoClicker(*handler);
                    }
                }
                else if (handler->getPaused())
                {
                    handler->setPaused(false);
                    handler->m_cv.notify_all();
                }
                updateStatusIndicator(keyInfo.name, handler->getActive());
            }
            else
            {
                updateStatusIndicator(keyInfo.name);
            }
            return true;
        }
        case ControlAction::Toggle:
        {
            InputHandler* handler = getHandlerForKeyInfo(keyInfo);
            if (handler)
            {
                if (!handler->getActive())
                {
                    if (isApplicationBlacklisted() || !isTargetApplicationActive())
                    {
                        return false;
                    }
                    if (!keyInfo.exclusiveGroup.empty())
                    {
                        stopExclusiveGroup(keyInfo.exclusiveGroup);
                    }
                    if (keyInfo.action == ActionState::HOLD)
                    {
                        handler->togglePressState();
                        handler->setActive(handler->isPressed());
                    }
                    else
                    {
                        startAutoClicker(*handler);
                    }
                }
                else
                {
                    if (keyInfo.action == ActionState::HOLD)
                    {
                        handler->togglePressState();
                        handler->setActive(handler->isPressed());
                    }
                    else
                    {
                        startAutoClicker(*handler);
                    }
                }
                updateStatusIndicator(keyInfo.name, handler->getActive());
            }
            else
            {
                updateStatusIndicator(keyInfo.name);
            }
            return true;
        }
        case ControlAction::Stop:
        case ControlAction::Cancel:
        {
            if (!keyInfo.name.empty())
            {
                stopCommand(keyInfo.name);
            }
            else if (InputHandler* handler = getHandlerForKeyInfo(keyInfo))
            {
                stopHandler(*handler);
            }
            return true;
        }
        case ControlAction::Pause:
        {
            if (!keyInfo.name.empty())
            {
                pauseCommand(keyInfo.name);
            }
            else if (InputHandler* handler = getHandlerForKeyInfo(keyInfo))
            {
                pauseHandler(*handler);
            }
            return true;
        }
        case ControlAction::Resume:
        {
            if (isApplicationBlacklisted() || !isTargetApplicationActive())
            {
                return false;
            }
            if (!keyInfo.name.empty())
            {
                resumeCommand(keyInfo.name);
            }
            else if (InputHandler* handler = getHandlerForKeyInfo(keyInfo))
            {
                resumeHandler(*handler);
            }
            return true;
        }
        case ControlAction::TogglePause:
        {
            if (!keyInfo.name.empty())
            {
                togglePauseCommand(keyInfo.name);
            }
            else if (InputHandler* handler = getHandlerForKeyInfo(keyInfo))
            {
                togglePauseHandler(*handler);
            }
            return true;
        }
        case ControlAction::StopAll:
        {
            stopAllCommands();
            return true;
        }
        case ControlAction::Exit:
        {
            exitRuntime();
            return true;
        }
        default:
            return false;
        }
    }

    bool Program::applyControlAction(const ControlAction action, const std::string_view name)
    {
        switch (action)
        {
        case ControlAction::Start:
        case ControlAction::Toggle:
        {
            if (isApplicationBlacklisted() || !isTargetApplicationActive())
            {
                return false;
            }
            if (!name.empty())
            {
                for (const auto& keyInfo : m_keyInfo)
                {
                    if (keyInfo.name == name)
                    {
                        KeyInfo infoCopy = keyInfo;
                        infoCopy.controlAction = action;
                        return applyControlAction(infoCopy);
                    }
                }

                if (InputHandler* handler = findHandlerByName(name))
                {
                    if (!handler->getExclusiveGroup().empty() && !handler->getActive())
                    {
                        stopExclusiveGroup(handler->getExclusiveGroup());
                    }
                    if (action == ControlAction::Start && handler->getActive())
                    {
                        if (handler->getPaused())
                        {
                            handler->setPaused(false);
                            handler->m_cv.notify_all();
                        }
                    }
                    else
                    {
                        startAutoClicker(*handler);
                    }
                    updateStatusIndicator(handler->getName(), handler->getActive());
                    return true;
                }
                return false;
            }
            return false;
        }
        case ControlAction::Stop:
        case ControlAction::Cancel:
        {
            if (!name.empty())
            {
                stopCommand(name);
                return true;
            }
            stopAllCommands();
            return true;
        }
        case ControlAction::Pause:
        {
            if (!name.empty())
            {
                pauseCommand(name);
                return true;
            }
            return false;
        }
        case ControlAction::Resume:
        {
            if (!name.empty())
            {
                resumeCommand(name);
                return true;
            }
            return false;
        }
        case ControlAction::TogglePause:
        {
            if (!name.empty())
            {
                togglePauseCommand(name);
                return true;
            }
            return false;
        }
        case ControlAction::StopAll:
        {
            stopAllCommands();
            return true;
        }
        case ControlAction::Exit:
        {
            exitRuntime();
            return true;
        }
        default:
            return false;
        }
    }

    void Program::runCommand(std::string_view name)
    {
        for (const KeyInfo& keyInfo : m_keyInfo)
        {
            if (keyInfo.name == name)
            {
                start(keyInfo);
                return;
            }
        }
    }

    void Program::stopCommand(const std::string_view name)
    {
        if (InputHandler* handler = findHandlerByName(name))
        {
            stopHandler(*handler);
        }
    }

    void Program::pauseCommand(const std::string_view name)
    {
        if (InputHandler* handler = findHandlerByName(name))
        {
            pauseHandler(*handler);
        }
    }

    void Program::resumeCommand(const std::string_view name)
    {
        if (InputHandler* handler = findHandlerByName(name))
        {
            resumeHandler(*handler);
        }
    }

    void Program::togglePauseCommand(const std::string_view name)
    {
        if (InputHandler* handler = findHandlerByName(name))
        {
            togglePauseHandler(*handler);
        }
    }

    void Program::stopAllCommands()
    {
        for (InputHandler* handler : getAllHandlers())
        {
            if (handler)
            {
                handler->release();
                if (handler->getActive() || handler->getPaused())
                {
                    handler->setActive(false);
                    handler->setPaused(false);
                    handler->m_autoclickerThread.request_stop();
                    handler->m_cv.notify_all();
                }
            }
        }

        m_keysPressed.clear();
        updateStatusIndicator();
    }

    void Program::exitRuntime()
    {
        stopAllCommands();
        platform::signalEnd();
        updateStatusIndicator();
    }

    void Program::end()
    {
        exitRuntime();
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

        for (InputHandler* handler : getAllHandlers())
        {
            if (handler)
            {
                handler->setPaused(shouldPause);
            }
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
        for (const InputHandler* handler : std::as_const(*this).getAllHandlers())
        {
            if (handler && handler->getActive() && !handler->getPaused())
            {
                isActive = true;
                break;
            }
        }

        const bool recording = m_recorder && (m_recorder->getState() == RecorderState::Recording || m_recorder->getState() == RecorderState::Paused);

        if (isActive != m_lastIsActiveIndicator || 
            (!triggeredCommandName.empty() && (triggeredCommandName != m_lastTriggeredCommandName || triggeredCommandActive != m_lastTriggeredCommandActive)) ||
            recording)
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

            if (m_statusCallback)
            {
                ProgramStatus status;
                status.active = isActive;
                status.triggeredCommandName = triggeredCommandName;
                status.triggeredCommandActive = triggeredCommandActive;
                
                if (m_recorder)
                {
                    status.recording = recording;
                    status.recordingPaused = m_recorder->getState() == RecorderState::Paused;
                    status.recordedEventCount = static_cast<uint32_t>(m_recorder->getSequence().events.size());
                }

                m_statusCallback(status);
            }

            m_lastIsActiveIndicator = isActive;
            m_lastTriggeredCommandName = triggeredCommandName;
            m_lastTriggeredCommandActive = triggeredCommandActive;
        }
    }

    // ReSharper disable once CppMemberFunctionMayBeConst
    void Program::startAutoClicker(InputHandler& handler) // NOLINT(*-make-member-function-const)
    {
        if (handler.getActive())
        {
            handler.release();
            handler.setActive(false);
            handler.m_autoclickerThread.request_stop();
            return;
        }

        auto delayData = m_arguments.delayData;
        handler.setActive(true);
        handler.m_autoclickerThread = std::jthread([&handler, delayData](const std::stop_token& stoken)
        {
            while (!stoken.stop_requested())
            {
                if (handler.getPaused())
                {
                    if (handler.isPressed())
                    {
                        handler.release();
                    }
                    std::unique_lock lock(handler.m_mutex);
                    handler.m_cv.wait_for(lock, stoken, std::chrono::milliseconds(100), [&]{ return stoken.stop_requested(); });
                    continue;
                }

                handler.press();
                const auto pressWaitTime = std::chrono::milliseconds(delayData.getPressDelay());
                Logger::debug("Pressed: {}, waiting: {}\n", handler.getName(), pressWaitTime);
                
                {
                    std::unique_lock lock(handler.m_mutex);
                    if (handler.m_cv.wait_for(lock, stoken, pressWaitTime, [&]{ return stoken.stop_requested(); }))
                    {
                        // Stop requested
                        if (handler.isPressed()) handler.release();
                        break;
                    }
                }

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
                
                {
                    std::unique_lock lock(handler.m_mutex);
                    handler.m_cv.wait_for(lock, stoken, releaseWaitTime, [&]{ return stoken.stop_requested(); });
                }
            }
        });
    }

    void Program::setStatusCallback(StatusCallback callback)
    {
        m_statusCallback = std::move(callback);
    }

    void Program::startRecording(const SequenceConfig& config)
    {
        m_recorder = std::make_unique<SequenceRecorder>(config);
        m_recorder->start();
        updateStatusIndicator();
    }

    void Program::stopRecording()
    {
        if (m_recorder)
        {
            m_recorder->stop();
            updateStatusIndicator();
        }
    }

    void Program::pauseRecording()
    {
        if (m_recorder)
        {
            m_recorder->pause();
            updateStatusIndicator();
        }
    }

    void Program::resumeRecording()
    {
        if (m_recorder)
        {
            m_recorder->resume();
            updateStatusIndicator();
        }
    }

    void Program::discardRecording()
    {
        if (m_recorder)
        {
            m_recorder->cancel();
            m_recorder.reset();
            updateStatusIndicator();
        }
    }

    const RecordedSequence* Program::getRecordedSequence() const
    {
        return m_recorder ? &m_recorder->getSequence() : nullptr;
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
