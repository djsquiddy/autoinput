/**
 * @file settings.h
 * @author djsquiddy
 * @date July 2026
 */

#ifndef INCLUDE_AUTOINPUT_SETTINGS_H
#define INCLUDE_AUTOINPUT_SETTINGS_H
#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <gsl/gsl>

#include "autoinput/config.h"

namespace autoinput
{
    class Settings
    {
    public:
        bool load(const std::optional<std::filesystem::path>& path = std::nullopt);

        [[nodiscard]] const DefaultSettings& getDefaults() const { return m_defaults; }

    private:
        bool loadFromFile(const std::filesystem::path& path);
        DefaultSettings m_defaults;
    };
}

#endif // INCLUDE_AUTOINPUT_SETTINGS_H
