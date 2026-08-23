/**
 * @file configMetadata.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_CONFIG_CONFIGMETADATA_H
#define INCLUDE_AUTOINPUT_CONFIG_CONFIGMETADATA_H
#pragma once

#include <vector>
#include <string_view>

namespace autoinput
{
    /**
     * @brief Provides metadata about valid configuration values.
     */
    class ConfigMetadata
    {
    public:
        /**
         * @brief Get a list of valid action names.
         * @return A vector of valid action names.
         */
        [[nodiscard]] static std::vector<std::string_view> validActionNames();

        /**
         * @brief Get a list of valid action names including aliases.
         * @return A vector of valid action names and aliases.
         */
        [[nodiscard]] static std::vector<std::string_view> validActionAliases();

        /**
         * @brief Get a string representing valid action choices for help text.
         * @return A string like "{click,c,hold,h}".
         */
        [[nodiscard]] static std::string validActionChoices();

        /**
         * @brief Get a list of valid control action names.
         * @return A vector of valid control action names.
         */
        [[nodiscard]] static std::vector<std::string_view> validControlActionNames();

        /**
         * @brief Get a list of valid control action names including aliases.
         * @return A vector of valid control action names and aliases.
         */
        [[nodiscard]] static std::vector<std::string_view> validControlActionAliases();

        /**
         * @brief Get a string representing valid control action choices for help text.
         * @return A string like "{start,toggle,stop,cancel,pause,resume,toggle-pause,stop-all,exit}".
         */
        [[nodiscard]] static std::string validControlActionChoices();

        /**
         * @brief Get the default control action name.
         * @return The default control action name.
         */
        [[nodiscard]] static std::string_view defaultControlActionName();

        /**
         * @brief Get a list of valid mouse button names.
         * @return A vector of valid mouse button names.
         */
        [[nodiscard]] static std::vector<std::string_view> validMouseButtonNames();

        /**
         * @brief Get a list of valid mouse button names including aliases.
         * @return A vector of valid mouse button names and aliases.
         */
        [[nodiscard]] static std::vector<std::string_view> validMouseButtonAliases();

        /**
         * @brief Get a string representing valid mouse button choices for help text.
         * @return A string like "{left,l,right,r,middle,m,back,forward}".
         */
        [[nodiscard]] static std::string validMouseButtonChoices();

        /**
         * @brief Get the default mouse button to use.
         * @return The default mouse button name.
         */
        [[nodiscard]] static std::string_view defaultMouseButtonName();

        /**
         * @brief Get the default action to perform.
         * @return The default action name.
         */
        [[nodiscard]] static std::string_view defaultActionName();

        /**
         * @brief Get the default start key.
         * @return The default start key name.
         */
        [[nodiscard]] static std::string_view defaultStartKey();

        /**
         * @brief Get the default end key.
         * @return The default end key name.
         */
        [[nodiscard]] static std::string_view defaultEndKey();

        /**
         * @brief Get a list of valid log level names.
         * @return A vector of valid log level names.
         */
        [[nodiscard]] static std::vector<std::string_view> validLogLevelNames();

        /**
         * @brief Get a list of valid log level names including aliases.
         * @return A vector of valid log level names and aliases.
         */
        [[nodiscard]] static std::vector<std::string_view> validLogLevelAliases();

        /**
         * @brief Get a string representing valid log level choices for help text.
         * @return A string like "{d,debug,i,info,w,warn,warning,e,error,f,fatal}".
         */
        [[nodiscard]] static std::string validLogLevelChoices();

        /**
         * @brief Get a list of valid special key names.
         * @return A vector of valid special key names.
         */
        [[nodiscard]] static std::vector<std::string_view> validSpecialKeyNames();

        /**
         * @brief Get a list of canonical wildcard input names.
         * @return A vector containing wildcard input triggers ("mouse.all", "keys.all", "input.all").
         */
        [[nodiscard]] static std::vector<std::string_view> validWildcardInputNames();

        /**
         * @brief Get a list of all wildcard input aliases.
         * @return A vector containing wildcard input triggers and aliases.
         */
        [[nodiscard]] static std::vector<std::string_view> validWildcardInputAliases();

        /**
         * @brief Checks if a trigger string represents any/all mouse buttons.
         * @param str Trigger string to check (e.g. "mouse.all", "mouse.*", "mouse.any").
         * @return True if the string matches a mouse wildcard.
         */
        [[nodiscard]] static bool isMouseAllTrigger(std::string_view str);

        /**
         * @brief Checks if a trigger string represents any/all keyboard keys.
         * @param str Trigger string to check (e.g. "keys.all", "key.all", "keys.*").
         * @return True if the string matches a keyboard key wildcard.
         */
        [[nodiscard]] static bool isKeysAllTrigger(std::string_view str);

        /**
         * @brief Checks if a trigger string represents any/all manual input (mouse or keyboard).
         * @param str Trigger string to check (e.g. "input.all", "input.*", "all", "any").
         * @return True if the string matches a global manual input wildcard.
         */
        [[nodiscard]] static bool isInputAllTrigger(std::string_view str);

        /**
         * @brief Checks if a trigger string represents any wildcard trigger.
         * @param str Trigger string to check.
         * @return True if the string matches any wildcard.
         */
        [[nodiscard]] static bool isWildcardTrigger(std::string_view str);
    };
}

#endif // INCLUDE_AUTOINPUT_CONFIG_CONFIGMETADATA_H
