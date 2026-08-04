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
        /**
         * @brief Default constructor.
         */
        SequenceHandler() = default;

        /**
         * @brief Constructs a SequenceHandler from a recorded sequence.
         * @param sequence The recorded sequence data.
         * @param backend Pointer to the platform backend.
         */
        SequenceHandler(RecordedSequence sequence, IPlatformBackend* backend);

        /**
         * @brief Copy constructor.
         */
        SequenceHandler(const SequenceHandler& rhs);

        /**
         * @brief Move constructor.
         */
        SequenceHandler(SequenceHandler&& rhs) noexcept;

        /**
         * @brief Copy assignment operator.
         */
        SequenceHandler& operator=(const SequenceHandler& rhs);

        /**
         * @brief Move assignment operator.
         */
        SequenceHandler& operator=(SequenceHandler&& rhs) noexcept;

        /**
         * @brief Toggles the playback of the sequence.
         */
        void togglePressState() override;

        /**
         * @brief Starts sequence playback.
         */
        void press() override;

        /**
         * @brief Stops sequence playback.
         */
        void release() override;

        /**
         * @brief Gets the name of the sequence.
         * @return The sequence name.
         */
        [[nodiscard]] std::string getTargetName() const override { return m_sequence.name; }

        /**
         * @brief Gets the recorded sequence data.
         * @return Const reference to RecordedSequence.
         */
        [[nodiscard]] const RecordedSequence& getSequence() const { return m_sequence; }

    private:
        RecordedSequence m_sequence;
        void playback();
    };
}

#endif // INCLUDE_AUTOINPUT_SEQUENCE_H
