/**
 * @file sequenceViewer.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_EDITORS_SEQUENCE_VIEWER_H
#define INCLUDE_AUTOINPUT_UI_EDITORS_SEQUENCE_VIEWER_H
#pragma once

#include <vector>

namespace autoinput
{
    struct RecordedSequence;
}

namespace autoinput::ui::editors
{
    /**
     * @brief Renders a read-only viewer for recorded sequences.
     * @param sequences The list of sequences to display.
     */
    void renderSequenceViewer(const std::vector<autoinput::RecordedSequence>& sequences);
}

#endif // INCLUDE_AUTOINPUT_UI_EDITORS_SEQUENCE_VIEWER_H
