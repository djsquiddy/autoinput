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

    /**
     * @brief The default log level.
     */
    constexpr std::string_view DefaultLogLevel = "info";

    /**
     * @brief The default key to start recording.
     */
    constexpr std::string_view RecordStartKey = "f8";

    /**
     * @brief The default key to end recording.
     */
    constexpr std::string_view RecordEndKey = "f9";

    /**
     * @brief The default key to start playing back a recorded sequence.
     */
    constexpr std::string_view RecordPlayStartKey = "f6";

    /**
     * @brief The default mouse movement sampling rate for recording.
     */
    constexpr std::string_view DefaultRecordMouseSample = "25ms";
}

#endif // INCLUDE_AUTOINPUT_DEFAULTS_H
