/**
 * @file sequence.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_SEQUENCE_H
#define INCLUDE_AUTOINPUT_SEQUENCE_H
#pragma once

#include "autoinput/handlerState.h"
#include "autoinput/config.h"
#include "autoinput/types.h"
#include <chrono>
#include <vector>

namespace autoinput
{
    class SequenceHandler : public InputHandler
    {
    public:
        SequenceHandler() = default;
        SequenceHandler(RecordedSequence sequence, IPlatformBackend* backend);
        SequenceHandler(const SequenceHandler& rhs);
        SequenceHandler(SequenceHandler&& rhs) noexcept;
        SequenceHandler& operator=(const SequenceHandler& rhs);
        SequenceHandler& operator=(SequenceHandler&& rhs) noexcept;

        void togglePressState() override;
        void press() override;
        void release() override;
        [[nodiscard]] std::string getTargetName() const override { return m_sequence.name; }
        [[nodiscard]] const RecordedSequence& getSequence() const { return m_sequence; }

    private:
        RecordedSequence m_sequence;
        void playback();
    };

    class SequenceRecorder
    {
    public:
        enum class State
        {
            Waiting,
            Recording,
            Finished,
            Cancelled
        };

        SequenceRecorder(std::string name, IPlatformBackend* backend);

        void start();
        void stop();
        void cancel();
        void recordKeyEvent(const Key& key, bool isDown);
        void recordMouseEvent(const Mouse& mouse, bool isDown, int32_t x, int32_t y);
        void recordMouseMove(int32_t x, int32_t y);

        [[nodiscard]] State getState() const { return m_state; }
        [[nodiscard]] const RecordedSequence& getSequence() const { return m_sequence; }

    private:
        State m_state{ State::Waiting };
        RecordedSequence m_sequence;
        IPlatformBackend* m_backend{ nullptr };
        std::chrono::steady_clock::time_point m_lastEventTime;

        void addEvent(RecordedEvent&& event);
    };
}

#endif // INCLUDE_AUTOINPUT_SEQUENCE_H
