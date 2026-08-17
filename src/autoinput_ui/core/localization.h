/**
 * @file localization.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_CORE_LOCALIZATION_H
#define INCLUDE_AUTOINPUT_UI_CORE_LOCALIZATION_H
#pragma once

#include "autoinput/support/types.h"
#include "autoinput/support/logger.h"
#include "autoinput/support/localizationIds.h"
#include <string>
#include <string_view>
#include <filesystem>
#include <map>
#include <format>
#include <set>
#include <vector>

namespace autoinput::ui
{
    /**
     * @brief Simple TOML-based localization system.
     */
    class Localization
    {
    public:
        Localization() = default;
        Localization(Localization&&) = delete;
        Localization(const Localization&) = delete;
        ~Localization() = default;
        Localization& operator=(Localization&&) = delete;
        Localization& operator=(const Localization&) = delete;

        /**
         * @brief Get the global localization instance.
         */
        static Localization& get();

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
         * @brief Get localized text by ID.
         * @param id The localization key ID (e.g. LocIds::APP_NAME_ID).
         * @return The localized string, or the key name if not found.
         */
        std::string_view text(LocId id) const;
        std::string_view text(const LocKey& locKey) const;

        /**
         * @brief Get localized text for a key, or a fallback if not found.
         * @param key The key.
         * @param fallback The fallback string.
         * @return The localized string, or the fallback.
         */
        std::string textOr(std::string_view key, std::string_view fallback) const;

        /**
         * @brief Get localized text by ID, or a fallback if not found.
         * @param id The localization key ID.
         * @param fallback The fallback string.
         * @return The localized string, or the fallback.
         */
        std::string_view textOr(LocId id, std::string_view fallback) const;
        std::string_view textOr(const LocKey& locKey, const std::string_view fallback) const { return textOr(locKey.index, fallback); }

        /**
         * @brief Check if a key exists.
         * @param key The key.
         * @return true if the key exists.
         */
        bool has(std::string_view key) const;

        /**
         * @brief Check if a key exists by ID.
         * @param id The localization key ID.
         * @return true if the key exists.
         */
        bool has(LocId id) const;
        bool has(const LocKey& locKey) const { return has(locKey.index); }

        /**
         * @brief Get localized text for a key with formatting.
         * @param key The key.
         * @param args The formatting arguments.
         * @return The formatted localized string.
         */
        template<typename... Args>
        std::string format(std::string_view key, Args&&... args) const;

        /**
         * @brief Get localized text by ID with formatting.
         * @param locKey The localization key ID.
         * @param args The formatting arguments.
         * @return The formatted localized string.
         */
        template<typename... Args>
        std::string format(const LocKey& locKey, Args&&... args) const;

        /**
         * @brief Get localized text by ID with formatting.
         * @param id The localization key ID.
         * @param args The formatting arguments.
         * @return The formatted localized string.
         */
        template<typename... Args>
        std::string format(LocId id, Args&&... args) const;

        /**
         * @brief Get a list of available languages (based on TOML files in resources).
         * @return Vector of language codes (e.g. ["en-US", "de-DE"]).
         */
        static std::vector<std::string> getAvailableLanguages();

    private:
        std::map<std::string, std::string, std::less<>> m_strings;
        std::array<std::string, LocalizationIds::KEY_COUNT> m_stringsById;
        std::array<uint8_t, LocalizationIds::KEY_COUNT> m_hasStringById;

        mutable std::set<std::string, std::less<>> m_missingKeys;
    };

    template <typename ... Args>
    std::string Localization::format(std::string_view key, Args&&... args) const
    {
        try
        {
            return std::vformat(text(key), std::make_format_args(args...));
        }
        catch (const std::format_error& e)
        {
            Logger::warn("Localization format error for key '{}': {}", key, e.what());
            return std::string{ text(key) };
        }
    }

    template <typename ... Args>
    std::string Localization::format(const LocKey& locKey, Args&&... args) const
    {
        if (!isFlagSet(locKey.settings, LocKeySettings::Format))
        {
            Logger::warn("Localization key '{}' is not marked as formattable", locKey.keyName);
            return std::string{ text(locKey) };
        }

        try
        {
            return std::vformat(text(locKey), std::make_format_args(args...));
        }
        catch (const std::format_error& e)
        {
            Logger::warn("Localization format error for ID {}: {}", locKey.index, e.what());
            return std::string{ text(locKey) };
        }
    }

    template <typename ... Args>
    std::string Localization::format(const LocId id, Args&&... args) const
    {
        try
        {
            return std::vformat(text(id), std::make_format_args(args...));
        }
        catch (const std::format_error& e)
        {
            Logger::warn("Localization format error for ID {}: {}", id, e.what());
            return std::string{ text(id) };
        }
    }
}

#endif // INCLUDE_AUTOINPUT_UI_CORE_LOCALIZATION_H
