#pragma once
#include <vector>
#include <cstdint>
//104C6510: transform channels first; then channels of each non-null slave.
// Collector callbacks must preserve native reverse-reference order and duplicates.
template<class CollectChannels,class CollectSlaves>
std::vector<uint32_t> gatherRecoveredControlChannelsCandidate(uint32_t dag,uint32_t transform,CollectChannels collectChannels,CollectSlaves collectSlaves){
 std::vector<uint32_t>out;collectChannels(out,transform);
 for(uint32_t slave:collectSlaves(dag))if(slave!=0xffffffffu)collectChannels(out,slave);
 return out;
}
