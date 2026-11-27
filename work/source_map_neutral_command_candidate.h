#pragma once

// Ordering only. Host supplies command representation, tick conversion,
// networking and command execution. This does not implement player movement.
template<class Host>
void sourceMapRunNeutralCommandCandidate(Host& host) {
    host.resetInputAccumulator();
    const auto savedClock=host.clock();
    auto command=host.neutralCommand(host.viewAngles());
    const auto tick=host.roundedServerTick();
    if(host.tickCounter()!=tick) host.setTickCounter(tick);
    host.setCommandHost(true);
    host.runCommand(command);
    host.storeLastCommand(command);
    host.restoreClock(savedClock);
    host.setCommandHost(false);
}
