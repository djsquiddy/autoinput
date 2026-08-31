/**
 * @file recorderGraphAdapter.cpp
 * @brief Implementation of recorder-to-graph integration and workflow adapter.
 * @author djsquiddy
 * @date August 2026
 */
#include "recorderGraphAdapter.h"
#include "autoinput/input/waitDelay.h"

#include <algorithm>
#include <format>

namespace autoinput::ui::graph
{
    autoinput::RecordedSequence sanitizeRecordedSequence(const autoinput::RecordedSequence& sequence)
    {
        autoinput::RecordedSequence sanitized = sequence;
        for (auto& ev : sanitized.events)
        {
            if (ev.delay.empty())
            {
                ev.delay = "0ms";
            }

            if (ev.type == autoinput::RecordedEventType::MouseMove)
            {
                if (!ev.x.has_value())
                {
                    ev.x = 0;
                }
                if (!ev.y.has_value())
                {
                    ev.y = 0;
                }
            }
        }
        return sanitized;
    }

    RecorderGraphGenerationResult generateGraphFromRecordedSequence(const autoinput::RecordedSequence& sequence,
                                                                    const SequenceGraphOptions& options)
    {
        RecorderGraphGenerationResult result;

        // Check for partial or warning conditions across recorded events
        for (std::size_t i = 0; i < sequence.events.size(); ++i)
        {
            const auto& ev = sequence.events[i];
            if (ev.type == autoinput::RecordedEventType::Invalid)
            {
                result.warnings.push_back(std::format("Event #{}: Unrecognized or invalid event type.", i));
            }
            else if (ev.type == autoinput::RecordedEventType::KeyDown || ev.type == autoinput::RecordedEventType::KeyUp)
            {
                if (!ev.key.has_value() || ev.key->empty())
                {
                    result.warnings.push_back(std::format("Event #{}: Keyboard event has empty or missing key.", i));
                }
            }
            else if (ev.type == autoinput::RecordedEventType::MouseDown ||
                     ev.type == autoinput::RecordedEventType::MouseUp)
            {
                if (!ev.button.has_value() || ev.button->empty())
                {
                    result.warnings.push_back(
                        std::format("Event #{}: Mouse button event has empty or missing button.", i));
                }
            }
            else if (ev.type == autoinput::RecordedEventType::MouseMove)
            {
                if (!ev.x.has_value() || !ev.y.has_value())
                {
                    result.warnings.push_back(
                        std::format("Event #{}: Mouse move event has incomplete coordinates (x={}, y={}).", i,
                                    ev.x.value_or(0), ev.y.value_or(0)));
                }
            }
        }

        // Generate graph document
        result.graphDocument = sequenceToGraphDocument(sequence, options);

        // Run validation
        auto valOptions = ValidationOptions::sequenceGraph();
        valOptions.treatDisconnectedAsError = true;
        result.validationResult = validateGraph(result.graphDocument, valOptions);

        result.success = result.validationResult.isValid();
        if (result.success)
        {
            result.statusMessage = std::format("Graph generated successfully ({} nodes, {} links).",
                                               result.graphDocument.nodeCount(), result.graphDocument.linkCount());
        }
        else
        {
            result.statusMessage =
                std::format("Graph generated with {} validation issue(s).", result.validationResult.issues.size());
        }

        return result;
    }

    void RecorderGraphWorkflow::onRecordingStarted(std::string_view sequenceName, std::string_view startKey,
                                                   std::string_view endKey)
    {
        (void)endKey;
        m_isRecording = true;
        m_isPaused = false;
        m_eventCount = 0;

        autoinput::RecordedSequence initialSeq;
        initialSeq.name = std::string(sequenceName.empty() ? "new_sequence" : sequenceName);
        initialSeq.start = std::string(startKey.empty() ? "f2" : startKey);
        initialSeq.repeat = false;

        m_recordedSequence = initialSeq;
        m_graphEditorState.rebuildFromSequence(initialSeq);
        m_statusMessage = "Recording started";
    }

    void RecorderGraphWorkflow::onRecordingUpdated(bool isRecording, bool isPaused, uint32_t eventCount,
                                                   std::optional<autoinput::RecordedSequence> currentSequence)
    {
        m_isRecording = isRecording;
        m_isPaused = isPaused;
        m_eventCount = eventCount;

        if (currentSequence.has_value())
        {
            m_recordedSequence = std::move(currentSequence);
            m_graphEditorState.syncWithSequence(*m_recordedSequence);
            m_statusMessage = std::format("Recording in progress ({} events)", m_eventCount);
        }
    }

    void RecorderGraphWorkflow::onRecordingStopped(std::optional<autoinput::RecordedSequence> finalSequence)
    {
        m_isRecording = false;
        m_isPaused = false;

        if (finalSequence.has_value())
        {
            m_recordedSequence = std::move(finalSequence);
            m_eventCount = static_cast<uint32_t>(m_recordedSequence->events.size());
            m_graphEditorState.rebuildFromSequence(*m_recordedSequence);
            m_statusMessage = std::format("Recording stopped ({} events, graph generated)", m_eventCount);
        }
        else if (m_recordedSequence.has_value())
        {
            m_graphEditorState.rebuildFromSequence(*m_recordedSequence);
            m_statusMessage = std::format("Recording stopped ({} events, graph generated)", m_eventCount);
        }
        else
        {
            m_statusMessage = "Recording stopped (no events captured)";
        }
    }

    void RecorderGraphWorkflow::onRecordingDiscarded()
    {
        m_isRecording = false;
        m_isPaused = false;
        m_eventCount = 0;
        m_recordedSequence = std::nullopt;
        m_graphEditorState = editors::SequenceGraphEditorState{};
        m_statusMessage = "Recording discarded";
    }

    bool RecorderGraphWorkflow::validateGraph()
    {
        return m_graphEditorState.validateCurrentGraph();
    }

    bool RecorderGraphWorkflow::applyGraphEdits()
    {
        if (!m_recordedSequence.has_value())
        {
            return false;
        }
        return m_graphEditorState.applyToSequence(*m_recordedSequence);
    }

    bool RecorderGraphWorkflow::saveToConfig(autoinput::ConfigData& configData,
                                             std::string_view targetSequenceName) const
    {
        if (!m_recordedSequence.has_value())
        {
            return false;
        }

        auto seqToSave = *m_recordedSequence;
        if (!targetSequenceName.empty())
        {
            seqToSave.name = std::string(targetSequenceName);
        }

        for (auto& seq : configData.sequences)
        {
            if (seq.name == seqToSave.name)
            {
                seq = seqToSave;
                return true;
            }
        }

        configData.sequences.push_back(std::move(seqToSave));
        return true;
    }

} // namespace autoinput::ui::graph
