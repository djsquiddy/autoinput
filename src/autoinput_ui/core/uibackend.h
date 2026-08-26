/**
 * @file uibackend.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_UI_BACKEND_H
#define INCLUDE_AUTOINPUT_UI_UI_BACKEND_H
#pragma once

#include <memory>

namespace autoinput::ui
{
    class IUiBackend
    {
    public:
        virtual ~IUiBackend() = default;
        virtual void init() = 0;
        virtual void shutdown() = 0;
        virtual void newFrame() = 0;
        virtual void render() = 0;
        [[nodiscard]] virtual bool windowShouldClose() const = 0;
    };

    std::unique_ptr<IUiBackend> createUiBackend();
}

#endif // INCLUDE_AUTOINPUT_UI_UI_BACKEND_H
