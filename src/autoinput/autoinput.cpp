//
// Created by djsquiddy on 3/9/2026.
//
#include "autoinput.h"

#include <format>
#include <iostream>
#include <ranges>
#include <random>

#include "platform.h"

namespace autoinput
{
    bool Program::processKeyEvent(const KeyboardInput& input)
    {
        if (!input.isKeyDown())
        {
            return false;
        }

        if (m_hasFunctionKeys)
        {
            if (const int32_t functionKey = input.functionKey(); functionKey != INVALID_KEY)
            {
                for (int32_t i = 0; i < m_keyInfo.size(); ++i)
                {
                    if (const KeyInfo& keyInfo = m_keyInfo[i]; keyInfo.functionKey == functionKey)
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
            if (const int32_t charKey = input.getChar(); charKey != INVALID_KEY)
            {
                for (int32_t i = 0; i < m_keyInfo.size(); ++i)
                {
                    if (const KeyInfo& keyInfo = m_keyInfo[i]; keyInfo.keyCode == charKey)
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
        if (m_arguments.buttonState == ButtonState::HOLD)
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
        for (int32_t i = 0; i < m_keyInfo.size(); ++i)
        {
            const KeyInfo& keyInfo = m_keyInfo[i];
            if (keyInfo.keyCode != INVALID_KEY)
            {
                if (keyInfo.mouseButton != MouseButton::NONE)
                {
                    std::cout << std::format("Key: {}, Is used as start key: {} to start {}", keyInfo.keyCode, keyInfo.isStartKey, mouseButtonToString(keyInfo.mouseButton)) << "\n";
                }
                else
                {
                    std::cout << std::format("Key: {}, Is used as start key: {}", keyInfo.keyCode, keyInfo.isStartKey) << "\n";
                }
            }
            else
            {
                if (keyInfo.mouseButton != MouseButton::NONE)
                {
                    std::cout << std::format("Function: F{}, Is used as start key: {} to start {}", keyInfo.functionKey, keyInfo.isStartKey, mouseButtonToString(keyInfo.mouseButton)) << "\n";
                }
                else
                {
                    std::cout << std::format("Function: F{}, Is used as start key: {}", keyInfo.functionKey, keyInfo.isStartKey) << "\n";
                }
            }
        }
        std::cout << std::endl;
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
                std::cout << std::format("Pressed button: {}, waiting: {}", handler.getButtonName(), pressWaitTime) << std::endl;
                std::this_thread::sleep_for(pressWaitTime);
                if (!handler.getActive())
                {
                    // Make sure we didn't just disable the callback.
                    break;
                }
                handler.releaseButton();
                const auto releaseWaitTime = std::chrono::milliseconds(delayData.getReleaseDelay());
                std::cout << std::format("Released button: {}, waiting: {}", handler.getButtonName(), releaseWaitTime) << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(releaseWaitTime));
            }
        });
    }

    void Program::init()
    {
        for (size_t i = 0; i < m_arguments.buttons.size(); ++i)
        {
            m_mouseHandlers[m_arguments.buttons[i]] = MouseHandler{m_arguments.buttons[i]};
            // m_mouseHandlers.insert(std::make_pair(m_arguments.buttons[i], MouseHandler{m_arguments.buttons[i]});
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
