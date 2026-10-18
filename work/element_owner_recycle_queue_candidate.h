#pragma once
#include <cstdint>
#include <cstring>
#include <cassert>
//10354c10 normal queue invariants; adapter supplies capacity/data growth.
template<class Adapter>
inline void enqueueElementOwnerSlotCandidate(std::uint32_t slot,Adapter& a) {
    if(a.tail==-1) {
        a.ensureOne();a.head=a.tail=0;
    } else {
        const auto oldCapacity=a.capacity();
        auto next=a.tail==oldCapacity-1?0:a.tail+1;
        if(next==a.head) {
            a.growOne();
            const auto added=a.capacity()-oldCapacity;
            next=a.tail==a.capacity()-1?0:a.tail+1;
            if(a.head!=0) {
                std::memmove(a.data()+a.head+added,a.data()+a.head,
                    static_cast<std::size_t>(oldCapacity-a.head)*sizeof(std::uint32_t));
                a.head+=added;
            }
        }
        a.tail=next;
    }
    a.data()[a.tail]=slot;
}
// Normal nonempty pop branch of1035abe0; caller reserves selected entry.
template<class Adapter>
inline std::uint32_t dequeueElementOwnerSlotCandidate(Adapter& a) {
    assert(a.head>=0 && a.tail>=0);
    const auto slot=a.data()[a.head];
    if(a.head==a.tail)a.head=a.tail=-1;
    else a.head=a.head==a.capacity()-1?0:a.head+1;
    return slot;
}
