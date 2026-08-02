/**
 * @file defaults.h
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_DEFAULTS_H
#define INCLUDE_AUTOINPUT_DEFAULTS_H
#pragma once

#include <string_view>
#include <cstdint>

namespace autoinput::defaults
{
    /**
     * @brief The default key to start the autoinput action.
     */
    constexpr std::string_view StartKey = "f2";

    /**
     * @brief The default key to end the autoinput action.
     */
    constexpr std::string_view EndKey = "f3";

    /**
     * @brief The default mouse button to use.
     */
    constexpr std::string_view DefaultMouseButtonName = "left";

    /**
     * @brief The default action to perform.
     */
    constexpr std::string_view DefaultActionName = "click";

    /**
     * @brief The default FPS for autoinput actions.
     */
    constexpr int32_t PressFps = 10;

    /**
     * @brief The default delay between autoinput actions in milliseconds.
     */
    constexpr int32_t DefaultDelay = 1000 / PressFps;

    /**
     * @brief The default status notification mode.
     */
    constexpr std::string_view DefaultStatusNotificationMode = "console";
}

#endif // INCLUDE_AUTOINPUT_DEFAULTS_H
