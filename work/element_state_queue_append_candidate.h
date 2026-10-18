#pragma once
#include <cstdint>
//1038f520 normal growth contract: allocator does not mutate logical count.
template<class Adapter>
inline void appendElementStateHandleCandidate(std::uint32_t handle,Adapter& a) {
    const auto oldCount=a.count();
    if(oldCount+1>a.capacity())a.grow(oldCount+1-a.capacity());
    a.setCount(oldCount+1);
    auto* data=a.data();a.setCached(data);
    data[oldCount]=handle;
}
