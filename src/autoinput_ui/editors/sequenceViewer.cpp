/**
 * @file sequenceViewer.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "sequenceViewer.h"
#include "autoinput/config.h"
#include <imgui.h>

namespace autoinput::ui::editors
{
    void renderSequenceViewer(const std::vector<autoinput::RecordedSequence>& sequences)
    {
        ImGui::Text("Recorded Sequences (View Only):");
        for (const auto& seq : sequences)
        {
            if (ImGui::CollapsingHeader(seq.name.c_str()))
            {
                ImGui::Text("Start Hotkey: %s", seq.start.c_str());
                ImGui::Text("Repeat: %s", seq.repeat ? "Yes" : "No");
                ImGui::Text("Events: %zu", seq.events.size());
            }
        }
        ImGui::TextDisabled("Full sequence editing TODO.");
    }
}
