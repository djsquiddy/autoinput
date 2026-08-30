/**
 * @file sequenceRecorder.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/input/sequenceRecorder.h"
#include "autoinput/support/logger.h"
#include "autoinput/input/waitDelay.h"
#include <format>
#include <fstream>

namespace autoinput
{
    SequenceRecorder::SequenceRecorder(SequenceConfig config)
        : m_config{ std::move(config) }
        , m_mouseSampleRateMs{ parseWaitDelay(m_config.mouseSampleDelay) }
    {
        m_sequence.name = m_config.name;
        m_sequence.start = m_config.playStartKey;
        m_sequence.repeat = false;
    }

    void SequenceRecorder::onKeyEvent(const Key& key, const bool isDown, const bool isSynthetic)
    {
        if (isSynthetic)
        {
            return;
        }

        const std::string keyStr = key.toString();
        if (m_state == RecorderState::Waiting)
        {
            if (keyStr == m_config.startKey && isDown)
            {
                start();
            }
            return;
        }

        if (m_state == RecorderState::Recording)
        {
            if (keyStr == m_config.endKey && isDown)
            {
                stop();
                return;
            }

            if (!m_config.recordKeyboardEvents)
            {
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
        if (isSynthetic || m_state != RecorderState::Recording || !m_config.recordMouseClicks)
        {
            return;
        }

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
        if (isSynthetic || !m_config.recordMouseMoves || m_state != RecorderState::Recording)
        {
            return;
        }

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

    void SequenceRecorder::start()
    {
        if (m_state == RecorderState::Waiting)
        {
            m_state = RecorderState::Recording;
            m_startTime = std::chrono::steady_clock::now();
            m_lastEventTime = m_startTime;
            Logger::info("Recording started...\n");
        }
    }

    void SequenceRecorder::pause()
    {
        if (m_state == RecorderState::Recording)
        {
            m_state = RecorderState::Paused;
            Logger::info("Recording paused.\n");
        }
    }

    void SequenceRecorder::resume()
    {
        if (m_state == RecorderState::Paused)
        {
            m_state = RecorderState::Recording;
            // Adjust last event time to avoid long pause gap
            m_lastEventTime = std::chrono::steady_clock::now();
            Logger::info("Recording resumed.\n");
        }
    }

    void SequenceRecorder::stop()
    {
        if (m_state == RecorderState::Recording || m_state == RecorderState::Paused)
        {
            m_state = RecorderState::Finished;
            Logger::info("Recording finished.\n");
        }
    }

    void SequenceRecorder::cancel()
    {
        m_state = RecorderState::Cancelled;
        m_sequence.events.clear();
        Logger::info("Recording cancelled.\n");
    }

    void SequenceRecorder::addEvent(RecordedEvent event)
    {
        m_sequence.events.emplace_back(std::move(event));
        m_lastEventTime = std::chrono::steady_clock::now();
    }

    std::string SequenceRecorder::getElapsedDelay() const
    {
        if (!m_config.recordDelays)
        {
            return "0ms";
        }
        const auto now = std::chrono::steady_clock::now();
        const auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastEventTime);
        return std::format("{}ms", diff.count());
    }

    bool SequenceRecorder::save(const std::filesystem::path& path, const bool force)const
    {
        if (std::filesystem::exists(path) && !force)
        {
            Logger::error("File already exists: {}. Use --force to overwrite.\n", path.string());
            return false;
        }

        ConfigData data;
        data.sequences.push_back(m_sequence);
        data.endKey = defaults::EndKey; // Default global end key for replaying

        if (saveConfigData(data, path, std::nullopt))
        {
            Logger::info("Sequence saved to {}\n", path.string());
            return true;
        }
        
        return false;
    }
}
