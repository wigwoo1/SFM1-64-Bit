#pragma once

#include "recovered_tick_seconds_candidate.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>

enum class SourceParticleCommandKindCandidate { Step, StartEmission, StopEmission };

struct SourceParticleCommandCandidate {
    SourceParticleCommandKindCandidate kind;
    int32_t localTick;
    float durationSeconds;
};

struct SourceParticleCollectionStepCandidate {
    int32_t cursor;
    bool complete;
    std::array<SourceParticleCommandCandidate, 3> commands{};
    size_t commandCount = 0;
};

// Advance an existing collection in the selected shot's local time. The
// caller owns collection creation, rewind/restart, and control-point history.
// Channel callbacks use that same clock and return the most recent key time
// when a corresponding Boolean state changes during a step.
template<class Simulating, class Emitting, class SimulatingKey, class EmittingKey>
SourceParticleCollectionStepCandidate sourceParticleCollectionStepCandidate(
        int32_t cursor, int32_t target, int32_t maxStepTicks, float simulationTimeScale,
        bool collectionEmitting, Simulating simulating, Emitting emitting,
        SimulatingKey simulatingKey, EmittingKey emittingKey) {
    if (target < cursor || maxStepTicks <= 0 || !std::isfinite(simulationTimeScale))
        throw std::invalid_argument("invalid existing particle collection step");
    auto end = int32_t(std::min<int64_t>(target, int64_t(cursor) + maxStepTicks));
    bool complete = end == target;
    const bool wasSimulating = simulating(cursor);
    const bool willSimulate = simulating(end);
    if (!wasSimulating && !willSimulate) return {target, true};
    if (wasSimulating != willSimulate) {
        const auto key = simulatingKey(end);
        if (!wasSimulating) return {std::max(cursor, key), false};
        end = std::max(cursor, std::min(end, key));
        complete = true;
    }

    SourceParticleCollectionStepCandidate result{complete ? target : end, complete};
    const auto step = [&](int32_t start, int32_t stop, int32_t pointTick) {
        result.commands[result.commandCount++] = {SourceParticleCommandKindCandidate::Step,
            pointTick, recoveredTickSecondsCandidate(int32_t(int64_t(stop) - start)) * simulationTimeScale};
    };
    const bool willEmit = emitting(end);
    if (collectionEmitting != willEmit) {
        const auto emissionTick = std::clamp(emittingKey(end), cursor, end);
        step(cursor, emissionTick, emissionTick);
        result.commands[result.commandCount++] = {
            willEmit ? SourceParticleCommandKindCandidate::StartEmission :
                       SourceParticleCommandKindCandidate::StopEmission, emissionTick, 0.f};
        if (int64_t(end) - emissionTick <= 1) {
            result.cursor = emissionTick;
            result.complete = emissionTick == target;
            return result;
        }
        // The second advance uses the same control-point pose. When the
        // collection is still catching up, Source advances its stored cursor
        // by the remaining duration, while a final step stores the target.
        if (!complete) result.cursor = int32_t(int64_t(cursor) + end - emissionTick);
        step(emissionTick, end, emissionTick);
        return result;
    }
    step(cursor, end, end);
    return result;
}
