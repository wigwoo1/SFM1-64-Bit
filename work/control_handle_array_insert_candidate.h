#pragma once
#include <cstdint>
struct ControlHandleArrayCandidate {
    std::uint32_t* data;
    std::int32_t capacity;
    std::int32_t count;
    std::uint32_t* cachedData;
};
// Valid index/count and successful growth are original caller preconditions.
// Callbacks retain original allocator/move policy and late source read.
template<class Grow,class Move,class ReadValue>
std::int32_t insertControlHandleCandidate(ControlHandleArrayCandidate& a,
    std::int32_t index,Grow grow,Move move,ReadValue readValue) {
    if (a.count+1>a.capacity) grow(a,a.count-a.capacity+1);
    ++a.count;
    a.cachedData=a.data;
    const auto trailing=a.count-index-1;
    if (trailing>0) move(a.data+index+1,a.data+index,trailing);
    if (a.data) a.data[index]=readValue();
    return index;
}
