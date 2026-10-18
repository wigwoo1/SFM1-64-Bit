#pragma once
#include <cstdint>
// 1038f160: each count is captured only when its corresponding pass begins.
// Data is reread at each iteration; appends beyond the snapshot are discarded
// by logical-count reset for the handle queues. Persistent callbacks remain.
template<class Adapter>
inline void processElementFrameworkPostQueuesCandidate(Adapter& a) {
    const auto postCount=a.postCount();
    for(std::int32_t i=0;i<postCount;++i)
        if(auto* e=a.resolve(a.postData()[i]))a.postUpdate(e);
    a.clearPostCount();
    const auto callbackCount=a.callbackCount();
    for(std::int32_t i=0;i<callbackCount;++i)a.invoke(a.callbackData()[i]);
    const auto resetCount=a.resetCount();
    for(std::int32_t i=0;i<resetCount;++i)
        if(auto* e=a.resolve(a.resetData()[i]))a.reset(e);
    a.clearResetCount();
}
