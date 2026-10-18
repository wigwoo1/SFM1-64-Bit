#pragma once
#include <cstdint>
//10353f20. Plain dirty flag/caller phase contract deliberately not generalized
// into a concurrency guarantee; adapter supplies actual storage and read locks.
template<class Adapter>
void publishOwnerSymbolPrimaryCandidate(Adapter& adapter) {
    if (!adapter.dirty()) return;
    for (std::uint32_t i=0;i<2048;++i) {
        auto* bucket=adapter.bucket(i);
        adapter.enterRead(bucket);
        adapter.setPrimary(bucket,adapter.secondary(bucket));
        adapter.leaveRead(bucket);
    }
    adapter.clearDirty();
}
