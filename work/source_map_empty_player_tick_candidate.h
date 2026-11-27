#pragma once

// Verified precondition: no command contexts/queued commands. This does not
// implement user commands, movement, physics or the special processing service.
template<class Host>
void sourceMapSimulateEmptyPlayerTickCandidate(Host& host) {
    host.enterProfile();
    if (host.hasValidParent()) host.simulateParent();
    if (host.lastTick()!=host.serverTick()) {
        host.setLastTick(host.serverTick());
        host.countQueuedTicks();
        if (host.specialProcessing()) host.processSpecial();
        host.clearEmptyCommandContexts();
    }
    host.exitProfile();
}
