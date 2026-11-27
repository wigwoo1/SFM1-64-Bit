#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

// Independent list/admission policy, not a physics simulator. Handles in the
// verified installed domain use indices below 8192. Entity lifetime/serial
// validation, allocation and dispatch remain the host's responsibility.
class SourceMapSimulationAdmissionCandidate {
public:
    struct Entry { uint16_t index; int tick; };
    static constexpr uint16_t absent=0xffff;
    std::array<uint16_t,8192> positions;
    std::vector<Entry> entries;
    SourceMapSimulationAdmissionCandidate() { positions.fill(absent); }

    template<class NextThink>
    void refresh(uint32_t handle,uint32_t flags,NextThink nextThink) {
        if ((flags&1) || handle==UINT32_MAX) return;
        const auto index=static_cast<uint16_t>(handle);
        if (index>=positions.size()) throw std::out_of_range("simulation entity index");
        auto slot=positions[index];
        if ((flags&0xc00000)==0xc00000) {
            if (slot!=absent) {
                entries[slot]=entries.back(); entries.pop_back(); positions[index]=absent;
                if (slot<entries.size()) positions[entries[slot].index]=slot;
            }
            return;
        }
        if (slot==absent) {
            slot=static_cast<uint16_t>(entries.size()); positions[index]=slot;
            entries.push_back({index,0});
        }
        entries[slot].tick=(flags&0x800000)?nextThink():0;
    }

    template<class Resolve>
    std::vector<uintptr_t> due(int tick,int capacity,Resolve resolve) const {
        std::vector<uintptr_t> selected;
        const int inspected=(std::min)(capacity,static_cast<int>(entries.size()));
        for (int i=0;i<inspected;++i)
            if (entries[i].tick<=tick) selected.push_back(resolve(entries[i].index));
        return selected;
    }
};

inline int sourceMapEarliestThinkTickCandidate(int baseTick,const std::vector<int>& contextTicks) {
    int result=baseTick>0?baseTick:-1;
    for (int tick:contextTicks) if (tick>0 && (result==-1 || tick<result)) result=tick;
    return result;
}
