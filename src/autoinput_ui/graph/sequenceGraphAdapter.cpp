/**
 * @file sequenceGraphAdapter.cpp
 * @brief Converter and adapter implementation transforming RecordedSequence into a GraphDocument.
 * @author djsquiddy
 * @date August 2026
 */
#include "sequenceGraphAdapter.h"
#include "autoinput/input/waitDelay.h"

#include <format>

namespace autoinput::ui::graph
{
    bool isNonZeroDelay(std::string_view delay) noexcept
    {
        if (delay.empty())
        {
            return false;
        }
        if (delay == "0ms" || delay == "0s" || delay == "0" || delay == "0.0s" || delay == "0.0ms")
        {
            return false;
        }
        const auto duration = autoinput::parseWaitDelay(delay);
        return duration.count() > 0;
    }

    std::string formatRecordedEventTitle(const autoinput::RecordedEvent& event)
    {
        switch (event.type)
        {
        case autoinput::RecordedEventType::KeyDown: return "Key Down";
        case autoinput::RecordedEventType::KeyUp: return "Key Up";
        case autoinput::RecordedEventType::MouseDown: return "Mouse Down";
        case autoinput::RecordedEventType::MouseUp: return "Mouse Up";
        case autoinput::RecordedEventType::MouseMove: return "Mouse Move";
        case autoinput::RecordedEventType::Invalid:
        default: return "Invalid Event";
        }
    }

    std::string formatRecordedEventSubtitle(const autoinput::RecordedEvent& event, bool includeDelay)
    {
        std::string result;
        switch (event.type)
        {
        case autoinput::RecordedEventType::KeyDown:
        case autoinput::RecordedEventType::KeyUp:
            if (event.key.has_value() && !event.key->empty())
            {
                result = std::format("Key: {}", *event.key);
            }
            else
            {
                result = "Key: <none>";
            }
            break;
        case autoinput::RecordedEventType::MouseDown:
        case autoinput::RecordedEventType::MouseUp:
            if (event.button.has_value() && !event.button->empty())
            {
                result = std::format("Button: {}", *event.button);
            }
            else
            {
                result = "Button: <none>";
            }
            break;
        case autoinput::RecordedEventType::MouseMove:
            result = std::format("Position: ({}, {})", event.x.value_or(0), event.y.value_or(0));
            break;
        case autoinput::RecordedEventType::Invalid:
        default: result = "Unknown event"; break;
        }

        if (includeDelay && isNonZeroDelay(event.delay))
        {
            if (result.empty())
            {
                result = std::format("Delay: {}", event.delay);
            }
            else
            {
                result += std::format(" (delay: {})", event.delay);
            }
        }

        return result;
    }

    GraphDocument sequenceToGraphDocument(const autoinput::RecordedSequence& sequence,
                                          const SequenceGraphOptions& options)
    {
        GraphDocument doc;
        std::size_t layoutIndex = 0;

        auto nextPosition = [&]() -> NodePosition
        {
            const float x = options.startX + (static_cast<float>(layoutIndex) * options.stepX);
            const float y = options.startY + (static_cast<float>(layoutIndex) * options.stepY);
            ++layoutIndex;
            return NodePosition{ .x = x, .y = y };
        };

        // 1. Create Start node
        auto& startNode = doc.createNode(NodeKind::Start, "Start", nextPosition());
        if (!sequence.start.empty())
        {
            startNode.setDetails(std::format("Trigger: {}", sequence.start));
        }
        else if (!sequence.name.empty())
        {
            startNode.setDetails(sequence.name);
        }
        auto* startOutPin = doc.createPin(startNode.id, PinDirection::Output, "Out");
        if (startOutPin == nullptr)
        {
            return doc;
        }
        PinId previousOutPinId = startOutPin->id;

        // 2. Iterate through events and generate nodes
        for (std::size_t i = 0; i < sequence.events.size(); ++i)
        {
            const auto& event = sequence.events[i];

            // Optional separated Wait node
            if (options.separateWaitNodes && isNonZeroDelay(event.delay))
            {
                auto& waitNode = doc.createNode(NodeKind::Wait, "Wait", nextPosition());
                waitNode.setDetails(std::format("Delay: {}", event.delay));
                waitNode.sourceIndex = i;

                auto* waitIn = doc.createPin(waitNode.id, PinDirection::Input, "In");
                auto* waitOut = doc.createPin(waitNode.id, PinDirection::Output, "Out");

                if (waitIn != nullptr && waitOut != nullptr)
                {
                    doc.createLink(previousOutPinId, waitIn->id);
                    previousOutPinId = waitOut->id;
                }
            }

            // Event node
            auto& eventNode = doc.createNode(NodeKind::RecordedEvent, formatRecordedEventTitle(event), nextPosition());
            eventNode.setDetails(formatRecordedEventSubtitle(event, !options.separateWaitNodes));
            eventNode.sourceIndex = i;

            auto* eventIn = doc.createPin(eventNode.id, PinDirection::Input, "In");
            auto* eventOut = doc.createPin(eventNode.id, PinDirection::Output, "Out");

            if (eventIn != nullptr && eventOut != nullptr)
            {
                doc.createLink(previousOutPinId, eventIn->id);
                previousOutPinId = eventOut->id;
            }
        }

        // 3. Create End node
        auto& endNode = doc.createNode(NodeKind::End, "End", nextPosition());
        endNode.setDetails(sequence.repeat ? "Repeat: Enabled" : "Repeat: Disabled");
        auto* endInPin = doc.createPin(endNode.id, PinDirection::Input, "In");
        if (endInPin != nullptr)
        {
            doc.createLink(previousOutPinId, endInPin->id);
        }

        return doc;
    }

} // namespace autoinput::ui::graph
