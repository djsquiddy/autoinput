/**
 * @file sequenceRecorder.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/sequenceRecorder.h"
#include "autoinput/logger.h"
#include "autoinput/waitDelay.h"
#include <format>
#include <fstream>

namespace autoinput
{
    SequenceRecorder::SequenceRecorder(std::string name, std::string startKey, std::string endKey, std::string playStartKey, bool recordMouseMoves, std::string mouseSampleDelay)
        : m_name(std::move(name)), m_startKey(std::move(startKey)), m_endKey(std::move(endKey)), m_playStartKey(std::move(playStartKey)), 
          m_recordMouseMoves(recordMouseMoves), m_mouseSampleDelay(std::move(mouseSampleDelay))
    {
        m_mouseSampleRateMs = parseWaitDelay(m_mouseSampleDelay);
        m_sequence.name = m_name;
        m_sequence.start = m_playStartKey;
        m_sequence.repeat = false;
    }

    void SequenceRecorder::onKeyEvent(const Key& key, const bool isDown, const bool isSynthetic)
    {
        if (isSynthetic) return;

        const std::string keyStr = key.toString();
        if (m_state == RecorderState::Waiting)
        {
            if (keyStr == m_startKey && isDown)
            {
                m_state = RecorderState::Recording;
                m_startTime = std::chrono::steady_clock::now();
                m_lastEventTime = m_startTime;
                Logger::info("Recording started...\n");
            }
            return;
        }

        if (m_state == RecorderState::Recording)
        {
            if (keyStr == m_endKey && isDown)
            {
                m_state = RecorderState::Finished;
                Logger::info("Recording finished.\n");
                return;
            }

            RecordedEvent event;
            event.type = isDown ? RecordedEventType::KeyDown : RecordedEventType::KeyUp;
            event.key = keyStr;
            event.delay = getElapsedDelay();
            addEvent(std::move(event));
        }
    }

    void SequenceRecorder::onMouseEvent(const Mouse& mouse, const bool isDown, const int32_t x, const int32_t y, const bool isSynthetic)
    {
        if (isSynthetic || m_state != RecorderState::Recording) return;

        RecordedEvent event;
        event.type = isDown ? RecordedEventType::MouseDown : RecordedEventType::MouseUp;
        event.button = mouse.toString();
        event.x = x;
        event.y = y;
        event.delay = getElapsedDelay();
        addEvent(std::move(event));
    }

    void SequenceRecorder::onMouseMove(const int32_t x, const int32_t y, const bool isSynthetic)
    {
        if (isSynthetic || !m_recordMouseMoves || m_state != RecorderState::Recording) return;

        const auto now = std::chrono::steady_clock::now();
        if (m_lastMouseMoveTime != std::chrono::steady_clock::time_point{} && (now - m_lastMouseMoveTime) < m_mouseSampleRateMs)
        {
            return;
        }

        RecordedEvent event;
        event.type = RecordedEventType::MouseMove;
        event.x = x;
        event.y = y;
        event.delay = getElapsedDelay();
        addEvent(std::move(event));
        m_lastMouseMoveTime = now;
    }

    void SequenceRecorder::addEvent(RecordedEvent event)
    {
        m_sequence.events.emplace_back(std::move(event));
        m_lastEventTime = std::chrono::steady_clock::now();
    }

    std::string SequenceRecorder::getElapsedDelay()
    {
        const auto now = std::chrono::steady_clock::now();
        const auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastEventTime);
        return std::format("{}ms", diff.count());
    }

    bool SequenceRecorder::save(const std::filesystem::path& path, const bool force)
    {
        if (std::filesystem::exists(path) && !force)
        {
            Logger::error("File already exists: {}. Use --force to overwrite.\n", path.string());
            return false;
        }

        ConfigData data;
        data.sequences.push_back(m_sequence);
        data.endKey = "f3"; // Default global end key for replaying

        if (saveConfigData(data, path, std::nullopt))
        {
            Logger::info("Sequence saved to {}\n", path.string());
            return true;
        }
        
        return false;
    }
}
