#pragma once

#include <cmath>
#include <cstdint>
#include <stdexcept>

struct SourceMapServerFrameCandidate {
    int32_t tick;
    float time;
    float frameTime;
    bool simulating;
};

// The local server publishes the authoritative tick before invoking GameFrame.
// The server tick interval and host step duration come from separate inputs.
inline SourceMapServerFrameCandidate sourceMapServerFrameCandidate(
    int32_t tick, float serverInterval, float hostStep, bool requested,
    int32_t maxClients, int32_t localSignonState) {
    if (tick < 0 || !std::isfinite(serverInterval) || serverInterval <= 0.f ||
        !std::isfinite(hostStep) || hostStep < 0.f) {
        throw std::invalid_argument("invalid local server frame inputs");
    }
    const float time = static_cast<float>(double(tick) * double(serverInterval));
    if (!std::isfinite(time)) throw std::invalid_argument("unrepresentable local server time");
    return {tick, time, requested ? hostStep : 0.f,
        requested && (maxClients > 1 || localSignonState == 6)};
}

struct SourceMapGameFrameCandidate {
    float frameTime;
    bool updateClientsOnly;
    bool runPhysics;
    bool simulatingPhysics;
    bool serviceEvents;
};

// A call to the physics routine in a nonsimulating frame can still maintain
// entities. Its simulation flag is independent of whether the event queue runs.
inline SourceMapGameFrameCandidate sourceMapGameFrameCandidate(
    const SourceMapServerFrameCandidate& frame, bool restoring,
    int32_t maxClients, bool alternateTicks) {
    if (restoring) return {frame.frameTime, false, false, false, false};
    const bool alternate = maxClients == 1 && alternateTicks;
    if (alternate && (frame.tick & 1)) return {frame.frameTime, true, false, false, false};
    return {alternate ? frame.frameTime * 2.f : frame.frameTime,
        false, true, frame.simulating, true};
}

// Gate for the next already-ordered event. Target lookup, execution and
// removal are handled by the queue owner.
inline bool sourceMapEventDueCandidate(
    float serverTime, float fireTime, bool steppingEvents, int32_t stepCredits) {
    return (!steppingEvents || stepCredits > 0) && serverTime >= fireTime;
}
