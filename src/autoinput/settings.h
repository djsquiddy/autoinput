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
#include <optional>
#include <filesystem>
#include <gsl/gsl>

#include "autoinput/config.h"

namespace autoinput
{
    struct SettingKey
    {
        std::string key{};
        std::string description{};
    };

    class Settings
    {
    public:
        /**
         * @brief Loads settings from a configuration file.
         * @param path Optional path to the settings file. If not provided, default paths are searched.
         * @return True if loading was successful.
         */
        bool load(const std::optional<std::filesystem::path>& path = std::nullopt);

        /**
         * @brief Gets the default settings.
         * @return Const reference to DefaultSettings.
         */
        [[nodiscard]] const DefaultSettings& getDefaults() const { return m_defaults; }


    private:
        bool loadFromFile(const std::filesystem::path& path);
        static const std::vector<SettingKey> s_keys;
        DefaultSettings m_defaults;
    };
}

#endif // INCLUDE_AUTOINPUT_SETTINGS_H
