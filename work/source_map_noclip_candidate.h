#pragma once

#include <cstdint>
#include <optional>

enum class SourceMapNoclipChange { unchanged, enter, leave };

// The caller supplies the parsed command argument, if one was given.
inline SourceMapNoclipChange sourceMapNoclipChange(
    bool cheats, bool commandPlayerAccepted, uint8_t moveType, std::optional<int> argument) {
    if (!cheats || !commandPlayerAccepted) return SourceMapNoclipChange::unchanged;
    const bool flying = moveType == 8;
    const bool requested = argument ? *argument != 0 : !flying;
    if (requested == flying) return SourceMapNoclipChange::unchanged;
    return requested ? SourceMapNoclipChange::enter : SourceMapNoclipChange::leave;
}
