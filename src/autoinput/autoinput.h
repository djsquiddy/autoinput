/**
 * @file autoInput.h
 * @author djsquiddy
 * @date March 2026
 */
#ifndef INCLUDE_AUTOINPUT_AUTOINPUT_H
#define INCLUDE_AUTOINPUT_AUTOINPUT_H
#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <thread>

#include "autoinput/arguments.h"
#include "autoinput/backend.h"
#include "autoinput/mouse.h"
#include "autoinput/keyboard.h"
#include "autoinput/keyInfo.h"
#include "autoinput/handlerState.h"
#include "autoinput/types.h"
#include "autoinput/notifications.h"

namespace autoinput
{
    class Program
    {
    public:
        Program() = default;
        explicit Program(std::unique_ptr<IPlatformBackend> backend);

        ProgramArguments& arguments() { return m_arguments; }
        void setBackend(std::unique_ptr<IPlatformBackend> backend);
        [[nodiscard]] IPlatformBackend* getBackend() const { return m_backend.get(); }
        std::unique_ptr<IPlatformBackend> releaseBackend() { return std::move(m_backend); }

        bool init();
        
        bool installHooks();
        void runListener();
        void cleanup();

        bool processKeyEvent(KeyboardInput&& input);
        bool processMouseEvent(const MouseInput& input);
        void start(const KeyInfo& keyInfo);
        void end();
        void joinThreads();

        [[nodiscard]] const std::vector<KeyInfo>& getKeyInfo() const { return m_keyInfo; }

#ifdef AUTOINPUT_TESTING
        auto& getMouseHandlers() { return m_mouseHandlers; }
        auto& getKeyHandlers() { return m_keyHandlers; }
        [[nodiscard]] bool getLastIsActiveIndicator() const { return m_lastIsActiveIndicator; }
#endif

        void printProgramInfo() const;
        [[nodiscard]] bool isApplicationBlacklisted() const;
        void onFocusChanged(const std::string& activeApp);
        void updateStatusIndicator();

    private:
        std::unique_ptr<IPlatformBackend> m_backend{ nullptr };
        std::unordered_map<Mouse, MouseHandler, HashFunction<Mouse>> m_mouseHandlers{};
        std::unordered_map<Key, KeyHandler, HashFunction<Key>> m_keyHandlers{};
        ProgramArguments m_arguments{};
        std::vector<KeyInfo> m_keyInfo{};
        std::vector<std::unique_ptr<std::thread>> m_zombieThreads{};
        bool m_lastIsActiveIndicator{ false };
        std::unique_ptr<NotificationService> m_notificationService{ nullptr };

        void startAutoClicker(InputHandler& handler);
    };

    bool installHooks();
    void runListener();
    void cleanup();

    inline std::unique_ptr<Program> g_program{ nullptr };
}

#endif // INCLUDE_AUTOINPUT_AUTOINPUT_H