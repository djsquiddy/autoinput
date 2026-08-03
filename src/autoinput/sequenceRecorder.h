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
        Finished,
        Cancelled
    };

    class SequenceRecorder
    {
    public:
        SequenceRecorder(std::string name, std::string startKey, std::string endKey, std::string playStartKey, bool recordMouseMoves, std::string mouseSampleDelay);

        void onKeyEvent(const Key& key, bool isDown, bool isSynthetic = false);
        void onMouseEvent(const Mouse& mouse, bool isDown, int32_t x, int32_t y, bool isSynthetic = false);
        void onMouseMove(int32_t x, int32_t y, bool isSynthetic = false);

        [[nodiscard]] RecorderState getState() const { return m_state; }
        [[nodiscard]] const RecordedSequence& getSequence() const { return m_sequence; }

        bool save(const std::filesystem::path& path, bool force);

    private:
        std::string m_name;
        std::string m_startKey;
        std::string m_endKey;
        std::string m_playStartKey;
        bool m_recordMouseMoves;
        std::string m_mouseSampleDelay;
        std::chrono::milliseconds m_mouseSampleRateMs;
        
        RecorderState m_state{ RecorderState::Waiting };
        RecordedSequence m_sequence;
        
        std::chrono::steady_clock::time_point m_startTime;
        std::chrono::steady_clock::time_point m_lastEventTime;
        std::chrono::steady_clock::time_point m_lastMouseMoveTime;

        void addEvent(RecordedEvent event);
        std::string getElapsedDelay();
    };
}

#endif // INCLUDE_AUTOINPUT_SEQUENCE_RECORDER_H
