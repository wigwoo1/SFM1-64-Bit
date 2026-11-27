#pragma once
#include <cstdint>

// Host supplies actual network-field updates, target services and movement.
// This policy does not implement those services or imply a live game actor.
template<class Host>
bool sourceMapSetObserverMode(int requested,Host& host) {
    if (static_cast<uint32_t>(requested)>6) return false;
    if (host.team()>1) {
        if (host.lifeStateIsTwo() && requested>3 && host.forceCamera()) requested=5;
        else if (requested==6) requested=4;
    }
    const int previous=host.mode();
    if (previous>1) host.setLastMode(previous);
    if (previous!=requested) host.setMode(requested);
    host.markModeTime();
    if (host.restrictedView() && requested!=1 && requested!=2 && host.team()>1) {
        const int fallback=host.accepts(host.observerTarget())?4:1;
        if (host.mode()!=fallback) host.setMode(fallback);
    }
    const int mode=host.mode();
    switch (mode) {
    case 0: case 1: case 3:
        host.resetFov(); host.zeroViewOffset(); host.move(0); break;
    case 2: case 6:
        host.resetFov(); host.applyTarget(host.targetReference()); host.zeroViewOffset(); host.move(10); break;
    case 4: case 5:
        host.applyTarget(host.targetReference()); host.move(10); break;
    }
    host.checkSettings();
    return true;
}
