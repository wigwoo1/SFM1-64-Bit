#pragma once
#include <cstdint>
// Initializer105cb270 defaults. Array relationships require graph adapters.
struct ControlGroupDefaultsCandidate {
    std::uint32_t groupColor = 0xffc8c8c8u;
    std::uint32_t controlColor = 0xffc8c8c8u;
    bool visible = true;
    bool selectable = true;
    bool snappable = true;
};
