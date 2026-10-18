#pragma once
#include "element_owner_reservation_candidate.h"
#include "element_owner_recycle_queue_candidate.h"
#include "element_owner_bind_candidate.h"
#include "element_owner_release_candidate.h"
#include <vector>
#include <cassert>
// Single-thread host storage fixture. Not production allocator/manager ABI.
struct ElementOwnerLifecycleFixture {
    struct Queue {
        std::vector<std::uint32_t> slots;
        std::int32_t head=-1,tail=-1;
        auto capacity() {return static_cast<std::int32_t>(slots.size());}
        auto* data() {return slots.data();}
        void ensureOne() {if(slots.empty())slots.resize(1);}
        void growOne() {slots.resize(slots.size()*2);}
    } queue;
    std::vector<ElementOwnerPoolEntryCandidate> entries;
    std::uint32_t live=0;
    bool hasRecycled() {return queue.head!=-1;}
    auto popRecycled() {return dequeueElementOwnerSlotCandidate(queue);}
    std::uint32_t appendEntry() {const auto i=static_cast<std::uint32_t>(entries.size());entries.emplace_back();return i;}
    auto& entry(std::uint32_t i) {return entries.at(i);}
    void incrementLiveCount() {++live;}
    auto reserve(std::uint32_t highBit=0) {return reserveElementOwnerCandidate(highBit,*this);}
    void bind(std::uint32_t handle,ElementOwnerCounterCandidate* object) {
        bindElementOwnerCandidate(handle,entries.data(),static_cast<std::uint32_t>(entries.size()),live,object);
    }
    void release(std::uint32_t handle) {
        releaseElementOwnerCandidate(handle,entries.data(),static_cast<std::uint32_t>(entries.size()),live,
            [&](std::uint32_t i){enqueueElementOwnerSlotCandidate(i,queue);});
    }
    auto* resolve(std::uint32_t handle) {return resolveElementOwnerCandidate(handle,entries.data(),entries.size());}
};
inline void testElementOwnerLifecycleFixture() {
    ElementOwnerLifecycleFixture pool;ElementOwnerCounterCandidate objects[1000];
    std::vector<std::uint32_t> old;
    for(unsigned i=0;i<1000;++i) {
        auto handle=pool.reserve(i%2?0x80000000u:0);
        assert((handle&0xfffffu)==i && ((handle>>20)&0x7ffu)==0);
        pool.bind(handle,&objects[i]);assert(pool.resolve(handle)==&objects[i]);old.push_back(handle);
    }
    assert(pool.live==1000);
    for(unsigned i=0;i<1000;i+=2)pool.release(old[i]);
    assert(pool.live==500);
    for(unsigned i=0;i<1000;i+=2) {
        assert(pool.resolve(old[i])==nullptr);
        auto current=pool.reserve();assert(current==((1u<<20)|i));
        pool.bind(old[i],&objects[(i+1)%1000]);assert(pool.resolve(current)==nullptr);
        pool.bind(current,&objects[i]);assert(pool.resolve(current)==&objects[i]);
    }
    assert(pool.live==1000 && pool.entries.size()==1000 && !pool.hasRecycled());
    for(unsigned i=1;i<1000;i+=2)assert(pool.resolve(old[i])==&objects[i]);
    ElementOwnerLifecycleFixture retirement;auto handle=retirement.reserve();
    for(unsigned generation=0;generation<2047;++generation) {
        assert(((handle>>20)&0x7ffu)==generation);
        retirement.bind(handle,&objects[0]);retirement.release(handle);
        assert(retirement.resolve(handle)==nullptr && retirement.live==0);
        if(generation<2046) {handle=retirement.reserve();assert((handle&0xfffffu)==0);}
    }
    assert(!retirement.hasRecycled());
    assert(retirement.reserve()==1 && retirement.entries.size()==2);
}
