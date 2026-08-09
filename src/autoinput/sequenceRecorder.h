/**
 * @file sequenceRecorder.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_SEQUENCE_RECORDER_H
#define INCLUDE_AUTOINPUT_SEQUENCE_RECORDER_H
#pragma once

#include "autoinput/config.h"
#include "autoinput/types.h"
#include <chrono>
#include <vector>
#include <string>
#include <filesystem>
#include <optional>

namespace autoinput
{
    enum class RecorderState
    {
        Waiting,
        Recording,
        Paused,
        Finished,
        Cancelled
    };

    struct SequenceConfig
    {
        bool recordMouseMoves{ false };
        bool recordMouseClicks{ true };
        bool recordKeyboardEvents{ true };
        bool recordDelays{ true };
        std::string name{};
        std::string startKey{};
        std::string endKey{};
        std::string playStartKey{};
        std::string mouseSampleDelay{};
    };

    class SequenceRecorder
    {
    public:
        /**
         * @brief Constructs a SequenceRecorder with the given configuration.
         * @param config The recorder configuration.
         */
        explicit SequenceRecorder(SequenceConfig config);

        /**
         * @brief Handles a keyboard event during recording.
         * @param key The key involved.
         * @param isDown True if the key was pressed down.
         * @param isSynthetic True if the event was synthetically generated.
         */
        void onKeyEvent(const Key& key, bool isDown, bool isSynthetic = false);

        /**
         * @brief Handles a mouse button event during recording.
         * @param mouse The mouse button and modifiers.
         * @param isDown True if the button was pressed down.
         * @param x The x-coordinate of the mouse.
         * @param y The y-coordinate of the mouse.
         * @param isSynthetic True if the event was synthetically generated.
         */
        void onMouseEvent(const Mouse& mouse, bool isDown, int32_t x, int32_t y, bool isSynthetic = false);

        /**
         * @brief Handles a mouse movement event during recording.
         * @param x The x-coordinate.
         * @param y The y-coordinate.
         * @param isSynthetic True if the event was synthetically generated.
         */
        void onMouseMove(int32_t x, int32_t y, bool isSynthetic = false);

        /**
         * @brief Starts recording immediately.
         */
        void start();

        /**
         * @brief Pauses recording.
         */
        void pause();

        /**
         * @brief Resumes recording.
         */
        void resume();

        /**
         * @brief Stops recording and marks it as finished.
         */
        void stop();

        /**
         * @brief Cancels recording and discards events.
         */
        void cancel();

        /**
         * @brief Gets the current state of the recorder.
         * @return The RecorderState.
         */
        [[nodiscard]] RecorderState getState() const { return m_state; }

        /**
         * @brief Gets the recorded sequence.
         * @return Const reference to the RecordedSequence.
         */
        [[nodiscard]] const RecordedSequence& getSequence() const { return m_sequence; }

        /**
         * @brief Saves the recorded sequence to a file.
         * @param path The file path to save to.
         * @param force Whether to overwrite an existing file.
         * @return True if successful.
         */
        bool save(const std::filesystem::path& path, bool force);

    private:
        SequenceConfig m_config;
        std::chrono::milliseconds m_mouseSampleRateMs;
        
        RecorderState m_state{ RecorderState::Waiting };
        RecordedSequence m_sequence;
        
        std::chrono::steady_clock::time_point m_startTime;
        std::chrono::steady_clock::time_point m_lastEventTime;
        std::chrono::steady_clock::time_point m_lastMouseMoveTime;

        void addEvent(RecordedEvent event);
        [[nodiscard]] std::string getElapsedDelay() const;
    };
}

#endif // INCLUDE_AUTOINPUT_SEQUENCE_RECORDER_H
