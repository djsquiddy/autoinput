/**
 * @file autoInput.h
 * @author djsquiddy
 * @date March 2026
 */
#ifndef INCLUDE_AUTOINPUT_AUTOINPUT_H
#define INCLUDE_AUTOINPUT_AUTOINPUT_H
#pragma once

#include "arguments.h"
#include "mouse.h"
#include "keyboard.h"
#include "keyInfo.h"
#include "types.h"

namespace autoinput
{
    class Program
    {
    public:
        ProgramArguments& arguments() { return m_arguments; }
        void init();
        
        bool processKeyEvent(KeyboardInput&& input);
        bool processMouseEvent(const MouseInput& input);
        void start(const KeyInfo& keyInfo);
        void end();
        void joinThreads();

        [[nodiscard]] const std::vector<KeyInfo>& getKeyInfo() const { return m_keyInfo; }

#ifdef AUTOINPUT_TESTING
        auto& getMouseHandlers() { return m_mouseHandlers; }
        auto& getKeyHandlers() { return m_keyHandlers; }
#endif

        void printProgramInfo() const;
        [[nodiscard]] bool isApplicationBlacklisted() const;
        void onFocusChanged(const std::string& activeApp);

    private:
        std::unordered_map<Mouse, MouseHandler, HashFunction<Mouse>> m_mouseHandlers{};
        std::unordered_map<Key, KeyHandler, HashFunction<Key>> m_keyHandlers{};
        ProgramArguments m_arguments{};
        std::vector<KeyInfo> m_keyInfo{};
        std::vector<std::unique_ptr<std::thread>> m_zombieThreads{};

        void startAutoClicker(InputHandler& handler);
    };

    bool installHooks();
    void runListener();
    void cleanup();

    inline std::unique_ptr<Program> g_program{ nullptr };
}

#endif // INCLUDE_AUTOINPUT_AUTOINPUT_H