#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct SfmEditorTimelineSegmentCandidate {
    size_t startFrame = 0;
    size_t endFrame = 0;
    uint32_t element = 0;
    std::string label;
};

struct SfmEditorTimelineLaneCandidate {
    uint32_t group = UINT32_MAX;
    uint32_t track = UINT32_MAX;
    std::string groupName;
    std::string trackName;
    uint32_t clipType = 0;
    bool visible = true;
    std::vector<SfmEditorTimelineSegmentCandidate> segments;
};
