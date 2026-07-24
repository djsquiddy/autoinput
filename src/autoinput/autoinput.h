//
// Created by djsquiddy on 3/9/2026.
//

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
        void start(const KeyInfo& keyInfo);
        void end();

        void printProgramInfo() const;

    private:
        std::unordered_map<MouseButton, MouseHandler, HashFunction<MouseButton>> m_mouseHandlers{};
        ProgramArguments m_arguments{};
        std::vector<KeyInfo> m_keyInfo{};
        bool m_hasNonFunctionKeys{ false };
        bool m_hasFunctionKeys{ false };

        void startAutoClicker(MouseHandler& handler);
    };

    bool installHooks();
    void runListener();
    void cleanup();

    inline std::unique_ptr<Program> g_program{ nullptr };
}

#endif // INCLUDE_AUTOINPUT_AUTOINPUT_H