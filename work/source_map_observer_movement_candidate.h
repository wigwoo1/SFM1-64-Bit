#pragma once
#include <cstdint>

// Only the two movement requests issued by the verified observer-mode policy.
// Host owns physics objects, networking, simulation registration and entity lists.
// This is not a spatial relinker, collision solver or live-player implementation.
template<class Host>
bool sourceMapSetObserverMovement(int requested, int collide, Host& host) {
    if (requested!=0 && requested!=10) return false;
    const auto narrowed=static_cast<uint8_t>(collide);
    if (host.moveType()==requested) {
        if (host.moveCollide()!=narrowed) host.setMoveCollide(narrowed);
        return true;
    }
    if (host.moveType()==8) host.clearFlagFour();
    host.setMoveType(static_cast<uint8_t>(requested));
    if (host.moveCollide()!=narrowed) host.setMoveCollide(narrowed);
    host.physicsRulesChanged();
    if (host.predictionByte()!=1) host.setPredictionByte(1);
    if (host.simulationByte()!=0) host.setSimulationByte(0);
    // The following refresh overwrites the preceding prediction byte, even
    // when the final value equals its original value (dirty state still counts).
    const uint8_t enabled=host.simulationEnabledByte();
    if (host.predictionByte()!=enabled) host.setPredictionByte(enabled);
    if (bool(enabled)!=host.contextEnabled()) host.setContextEnabled(bool(enabled));
    // The installed player predicate at vtable +0x158 returns true. This
    // bounded player path therefore clears the exclusion bit for BOTH modes.
    if (host.excludedFromMovingList()) {
        host.clearMovingListExclusion();
        host.refreshMovingList();
    }
    return true;
}
