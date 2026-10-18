#pragma once
#include <cstdint>
template<class Adapter>
void notifyUndoPostActionListenersCandidate(Adapter& a) {
    const std::int32_t count=a.listenerCount();
    for(std::int32_t i=0;i<count;++i)a.invokeListenerSlot0(a.liveListener(i));
}
