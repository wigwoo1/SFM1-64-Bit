#pragma once

// Valid-player activation boundary. Host owns the real initial/spawn behavior,
// string-table service and notification. This does not select a spawn position.
template<class Host>
void sourceMapFinishClientActivation(Host& host) {
    const bool prior = host.lockStringTables(false);
    host.initialSpawn();
    host.spawn();
    host.lockStringTables(prior);
    auto text = host.announcementName(128);
    for (auto& character : text) {
        if (!character) break;
        if (character == '%') character = ' ';
    }
    if (!host.fakeClient() || host.extraClientFlag())
        host.announce(text[0] ? text.data() : "<unconnected>");
}
