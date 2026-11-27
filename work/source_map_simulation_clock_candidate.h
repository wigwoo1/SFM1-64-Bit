#pragma once

#include <cmath>
#include <cstdint>
#include <stdexcept>

struct SourceMapHostTickBudget {
    int32_t ticks;
    float remainder;
};

// The elapsed-time gate queries the engine's demo player. For a local
// recorded demo, a pause only holds elapsed time during active playback
// with an initialized demo cursor and no pending seek.
inline bool sourceMapRecordedDemoHoldsElapsed(
    bool playing, bool seeking, int32_t cursor, bool paused) {
    return playing && !seeking && cursor >= 0 && paused;
}

// Host-frame durations and server tick intervals are independent. In
// alternate-tick mode the engine rounds the ending tick up to an even tick,
// carrying any resulting time debt into the next host frame.
inline SourceMapHostTickBudget sourceMapHostTickBudget(
    float remainder, float hostDuration, float tickInterval, int32_t serverTick,
    bool addElapsed, bool alternateTicks) {
    if (!std::isfinite(remainder) || !std::isfinite(hostDuration) || hostDuration < 0.f ||
        !std::isfinite(tickInterval) || tickInterval <= 0.f || serverTick < 0) {
        throw std::invalid_argument("invalid host tick budget inputs");
    }
    const float available = addElapsed ? remainder + hostDuration : remainder;
    if (available < tickInterval) return {0, available};
    const float quotient = available / tickInterval;
    if (quotient >= 0x1p31f) throw std::overflow_error("host tick budget overflow");
    auto ticks = static_cast<int32_t>(quotient);
    if (alternateTicks && ((serverTick ^ ticks) & 1)) ++ticks;
    const float consumed = static_cast<float>(ticks) * tickInterval;
    return {ticks, available - consumed};
}

struct SourceMapServerFrameCandidate {
    int32_t tick;
    float time;
    float frameTime;
    bool simulating;
};

inline bool sourceMapServerAllowsStep(int32_t serverState, int32_t maxClients,
    int32_t localSignon, bool loading, bool consoleVisible, bool bugReporterPausesGame) {
    if (serverState < 2 || serverState == 3) return false;
    if (maxClients > 1) return true;
    return !loading && (localSignon != 6 || (!consoleVisible && !bugReporterPausesGame));
}

struct SourceMapServerDispatchCandidate {
    bool callGameFrame;
    bool advanceTick;
    bool requestSimulation;
};

// The server may maintain a multiplayer frame even with no pending step.
// Noclip during pause can advance the server clock without asking game
// physics to simulate that step.
inline SourceMapServerDispatchCandidate sourceMapServerDispatch(
    int32_t serverState, int32_t maxClients, int32_t pendingSteps, bool allowsStep,
    bool noclipDuringPause, bool gameBlocksTick) {
    if (serverState < 2) return {false, false, false};
    if (pendingSteps <= 0) return {maxClients > 1, false, false};
    return {true, !gameBlocksTick && (allowsStep || noclipDuringPause), allowsStep};
}

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
