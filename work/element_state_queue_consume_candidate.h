#pragma once
#include <cstdint>
template<class Adapter>
inline void consumeElementStateQueueCandidate(Adapter& a) {
    const auto count=a.count();a.setPhase(5);
    for(std::int32_t i=0;i<count;++i) {
        auto* element=a.resolve(a.data()[i]);
        if(element) {a.update(element);a.clearMarks(element);}
    }
    a.setCount(0);
}
