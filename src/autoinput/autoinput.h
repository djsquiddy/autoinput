/**
 * @file autoinput.h
 * @author djsquiddy
 * @date March 2026
 */
#ifndef INCLUDE_AUTOINPUT_AUTOINPUT_H
#define INCLUDE_AUTOINPUT_AUTOINPUT_H
#pragma once

#include "arguments.h"
#include "mouse.h"
#include "keyboard.h"
#include "keyinfo.h"
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

        [[nodiscard]] const std::vector<KeyInfo>& getKeyInfo() const { return m_keyInfo; }

        void printProgramInfo() const;

    private:
        std::unordered_map<MouseButton, MouseHandler, HashFunction<MouseButton>> m_mouseHandlers{};
        std::unordered_map<Key, KeyHandler, HashFunction<Key>> m_keyHandlers{};
        ProgramArguments m_arguments{};
        std::vector<KeyInfo> m_keyInfo{};

        void startAutoClicker(InputHandler& handler);
    };

    bool installHooks();
    void runListener();
    void cleanup();

    inline std::unique_ptr<Program> g_program{ nullptr };
}

#endif // INCLUDE_AUTOINPUT_AUTOINPUT_H