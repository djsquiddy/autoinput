/**
 * @file settings.h
 * @author djsquiddy
 * @date July 2026
 */

#ifndef INCLUDE_AUTOINPUT_SETTINGS_H
#define INCLUDE_AUTOINPUT_SETTINGS_H
#pragma once

#include "config.h"

namespace autoinput
{
    struct DefaultSettings
    {
        std::string start{};
        std::string end{};
        std::string press{};
        std::string release{};
        std::string action{};
        std::string button{};
    };

    class Settings
    {
    public:
        bool load(const std::optional<std::filesystem::path>& path = std::nullopt);

        [[nodiscard]] const DefaultSettings& getDefaults() const { return m_defaults; }

    private:
        DefaultSettings m_defaults;
    };
}

#endif // INCLUDE_AUTOINPUT_SETTINGS_H
