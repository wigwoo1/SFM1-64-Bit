#pragma once

// Creation boundary only. This does not select a spawn, activate the player,
// bind an edict, or simulate it. The allocator supplies a usable player object.
template<class Host>
void sourceMapCreateClientPlayer(Host& host) {
    if (host.hasOverride()) {
        host.overrideCreate();
        return;
    }
    auto created = host.create("player", -1, true);
    if (host.hasName()) host.setName(created, 32);
}
