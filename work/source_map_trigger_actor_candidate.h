#pragma once

#include <cstdint>

// The live entity table supplies the record for the client's handle index.
// The tool query compares the low ten serial bits used by network handles.
inline bool sourceMapToolClientMappingCandidate(
    uint32_t clientHandle, uint32_t serverSerial, bool serverEntityAvailable) {
    const uint32_t entityIndex = clientHandle & 0xffffu;
    return entityIndex < 2048 && serverEntityAvailable &&
        ((clientHandle >> 16) & 0x3ffu) == (serverSerial & 0x3ffu);
}

inline bool sourceMapToolLocalPlayerCandidate(
    int32_t maxClients, bool hasEdictArray, bool localEdictFree, bool localEntityAvailable) {
    return maxClients == 1 && hasEdictArray && !localEdictFree && localEntityAvailable;
}

// The original tool returns true when it cannot resolve either player.
inline bool sourceMapToolReportsNoclipCandidate(bool hasPlayer, uint8_t moveType) {
    return !hasPlayer || moveType == 8;
}

enum class SourceMapTouchScanCandidate : uint32_t { none, solidMoved, triggerMoved };

// This is the decision once the server's touch relink routine has been called.
// A movement mode by itself does not establish whether an actor is solid.
inline SourceMapTouchScanCandidate sourceMapTouchScanCandidate(
    bool hasEdict, bool isWorld, uint8_t solidType, uint16_t solidFlags, bool hasBrushModel) {
    constexpr uint8_t solidNone = 0;
    constexpr uint8_t solidBsp = 1;
    constexpr uint16_t notSolid = 4;
    constexpr uint16_t trigger = 8;
    using Scan = SourceMapTouchScanCandidate;
    if (!hasEdict || isWorld || (solidType == solidBsp && !hasBrushModel)) return Scan::none;
    if (solidFlags & trigger) return Scan::triggerMoved;
    return solidType != solidNone && !(solidFlags & notSolid) ? Scan::solidMoved : Scan::none;
}
