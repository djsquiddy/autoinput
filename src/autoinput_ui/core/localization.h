/**
 * @file localization.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_CORE_LOCALIZATION_H
#define INCLUDE_AUTOINPUT_UI_CORE_LOCALIZATION_H
#pragma once

#include <string>
#include <string_view>
#include <filesystem>
#include <map>
#include <memory>
#include <format>

namespace autoinput::ui
{
    /**
     * @brief Simple TOML-based localization system.
     */
    class Localization
    {
    public:
        Localization() = default;
        ~Localization() = default;

        /**
         * @brief Load localization strings from a TOML file.
         * @param path Path to the TOML file.
         * @param clearExisting If true, clear existing strings before loading.
         * @return true if loaded successfully.
         */
        bool loadFromFile(const std::filesystem::path& path, bool clearExisting = true);

        /**
         * @brief Get localized text for a key.
         * @param key The key (e.g. "app.name").
         * @return The localized string, or the key itself if not found.
         */
        std::string_view text(std::string_view key) const;

        /**
         * @brief Get localized text for a key, or a fallback if not found.
         * @param key The key.
         * @param fallback The fallback string.
         * @return The localized string, or the fallback.
         */
        std::string textOr(std::string_view key, std::string_view fallback) const;

        /**
         * @brief Check if a key exists.
         * @param key The key.
         * @return true if the key exists.
         */
        bool has(std::string_view key) const;

        /**
         * @brief Get localized text for a key with formatting.
         * @param key The key.
         * @param args The formatting arguments.
         * @return The formatted localized string.
         */
        template<typename... Args>
        std::string format(std::string_view key, Args&&... args) const
        {
            return std::vformat(text(key), std::make_format_args(args...));
        }

        /**
         * @brief Get the global localization instance.
         */
        static Localization& get();
 
        /**
         * @brief Get a list of available languages (based on TOML files in resources).
         * @return Vector of language codes (e.g. ["en-US", "de-DE"]).
         */
        static std::vector<std::string> getAvailableLanguages();

    private:
        std::map<std::string, std::string, std::less<>> m_strings;
    };
}

#endif // INCLUDE_AUTOINPUT_UI_CORE_LOCALIZATION_H
