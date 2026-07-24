//
// Created by djsquiddy on 3/9/2026.
//
#include "autoinput.h"
#include "logger.h"
#include "platform.h"

namespace autoinput
{
    bool Program::processKeyEvent(KeyboardInput&& input)
    {
        input.printInfo();

        if (!input.isKeyDown())
        {
            return false;
        }

        if (m_hasFunctionKeys)
        {
            if (const auto functionKey = input.functionKey(); functionKey != INVALID_KEY)
            {
                for (const KeyInfo& keyInfo : m_keyInfo)
                {
                    if ( keyInfo.functionKey == functionKey)
                    {
                        if (keyInfo.isStartKey)
                        {
                            start(keyInfo);
                            return true;
                        }

                        end();
                    }
                }
            }
        }
        if (m_hasNonFunctionKeys)
        {
            if (const auto charKey = input.getChar(); charKey != INVALID_KEY)
            {
                for (const KeyInfo& keyInfo : m_keyInfo)
                {
                    if (keyInfo.keyCode == charKey)
                    {
                        if (keyInfo.isStartKey)
                        {
                            start(keyInfo);
                            return true;
                        }

                        end();
                    }
                }
            }
        }

        return false;
    }

    void Program::start(const KeyInfo& keyInfo)
    {
        MouseHandler& handler = m_mouseHandlers.at(keyInfo.mouseButton);
        if (m_arguments.actionState == ActionState::HOLD)
        {
            handler.togglePressState();
        }
        else
        {
            startAutoClicker(handler);
        }
    }

    void Program::end()
    {
        for (auto& mouseHandler : m_mouseHandlers | std::views::values)
        {
            mouseHandler.releaseButton();
            mouseHandler.setActive(false);
        }

        platform::signalEnd();
    }

    void Program::printProgramInfo() const
    {
        // ReSharper disable once CppUseStructuredBinding
        for (auto keyInfo : m_keyInfo)
        {
            if (keyInfo.keyCode != INVALID_KEY)
            {
                if (keyInfo.mouseButton != MouseButton::NONE)
                {
                    Logger::info("Key: {}, Is used as start key: {} to start {}\n", keyInfo.keyCode, keyInfo.isStartKey, mouseButtonToString(keyInfo.mouseButton));
                }
                else
                {
                    Logger::info("Key: {}, Is used as start key: {}\n", keyInfo.keyCode, keyInfo.isStartKey);
                }
            }
            else
            {
                if (keyInfo.mouseButton != MouseButton::NONE)
                {
                    Logger::info("Function: F{}, Is used as start key: {} to start {}\n", keyInfo.functionKey, keyInfo.isStartKey, mouseButtonToString(keyInfo.mouseButton));
                }
                else
                {
                    Logger::info("Function: F{}, Is used as start key: {}\n", keyInfo.functionKey, keyInfo.isStartKey);
                }
            }
        }
        Logger::flush();
    }

    // ReSharper disable once CppMemberFunctionMayBeConst
    void Program::startAutoClicker(MouseHandler& handler) // NOLINT(*-make-member-function-const)
    {
        if (handler.getActive())
        {
            handler.releaseButton();
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

                handler.pressButton();
                const auto pressWaitTime = std::chrono::milliseconds(delayData.getPressDelay());
                Logger::debug("Pressed button: {}, waiting: {}", handler.getButtonName(), pressWaitTime);
                std::this_thread::sleep_for(pressWaitTime);
                if (!handler.getActive())
                {
                    // Make sure we didn't just disable the callback.
                    break;
                }
                handler.releaseButton();
                const auto releaseWaitTime = std::chrono::milliseconds(delayData.getReleaseDelay());
                Logger::debug("Released button: {}, waiting: {}", handler.getButtonName(), releaseWaitTime);
                std::this_thread::sleep_for(std::chrono::milliseconds(releaseWaitTime));
            }
        });
    }

    void Program::init()
    {
        for (auto & button : m_arguments.buttons)
        {
            m_mouseHandlers[button] = MouseHandler{button};
        }

        for (size_t i = 0; i < m_arguments.startKeys.size(); ++i)
        {
            const auto& key = m_arguments.startKeys[i];
            const auto ch = std::tolower(key[0]);
            if (key.length() == 1)
            {
                m_hasNonFunctionKeys = true;
                m_keyInfo.emplace_back(KeyInfo{
                    .keyCode = static_cast<int32_t>(ch),
                    .mouseButton = m_arguments.buttons[i],
                    .isStartKey = true
                });
            }
            else if (ch == 'f')
            {
                m_hasFunctionKeys = true;
                const int32_t result = parseStringToInt(key.substr(1));
                m_keyInfo.emplace_back(KeyInfo{
                    .functionKey = result,
                    .mouseButton = m_arguments.buttons[i],
                    .isStartKey = true
                });
            }
        }

        const auto ch = std::tolower(m_arguments.endKey[0]);
        if (m_arguments.endKey.length() == 1)
        {
            m_hasNonFunctionKeys = true;
            m_keyInfo.emplace_back(KeyInfo{
                .keyCode = static_cast<int32_t>(ch),
                .isStartKey = false
            });
        }
        else if (ch == 'f')
        {
            m_hasFunctionKeys = true;
            const int32_t result = parseStringToInt(m_arguments.endKey.substr(1));
            m_keyInfo.emplace_back(KeyInfo{
                .functionKey = result,
                .isStartKey = false
            });
        }
    }
}
