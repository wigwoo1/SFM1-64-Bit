#pragma once

// Entity lookup, eligibility and target application belong to the host.
// Read the stop flag after applying the target: callbacks may change it.
template<class Host>
void sourceMapSelectWelcomeObserver(Host& host) {
    auto point=host.next(0);
    while (point) {
        if (host.accepts(point)) host.select(point);
        if (host.stopAfter(point)) break;
        point=host.next(point);
    }
}
