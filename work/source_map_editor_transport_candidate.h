#pragma once

#include <cstdint>
#include "editor_adjust_time_candidate.h"

// The start/stop command changes the editor's playhead and frame-sync
// settings. This route describes ordinary film playback.
template<class Editor>
void sourceMapToggleEditorPlayback(Editor& editor, SfmFrameRateCandidate rate) {
    if (editor.playing) {
        editor.playbackSpeed = 1.f;
        if (editor.rewindAtEnd) {
            const int32_t frameDuration = sfmFrameToTimeCandidate(1.f, rate, 0,
                [](double time, unsigned mode) { return sfmDoubleTimeConvertCandidate(
                    time, mode, [](double) { return 0; }); });
            if (int64_t(editor.playhead) >= int64_t(editor.rangeEnd) - frameDuration - 1)
                editor.seek(editor.rangeStart);
        }
        if (editor.frameSyncPreferred && !editor.recording && !editor.otherPlaybackMode)
            editor.sync(true);
        editor.playing = false;
    } else {
        if (editor.frameSyncPreferred && !editor.recording && !editor.otherPlaybackMode) {
            const auto original = editor.playhead;
            editor.playhead = sfmAdjustTimeCandidate(original, rate, 0);
            editor.sync(false);
            if (editor.playhead != original) editor.playheadChanged();
        }
        editor.playbackSpeed = 1.f;
        editor.playing = true;
    }
    editor.playbackChanged();
}
