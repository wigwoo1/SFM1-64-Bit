#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
struct StudioDeviceQueuePublishCandidate{
 std::array<std::uintptr_t,500> ring{};
 std::uint32_t produced=0,completed=0,index=0;
};
// Single-thread policy oracle. Production synchronization remains external.
template<class Services> void studioDeviceQueuePublishCandidate(StudioDeviceQueuePublishCandidate& state,std::uintptr_t block,Services& services){
 for(;;){const std::uint32_t difference=state.produced-state.completed;if(difference<500u||difference>=0x80000000u)break;services.waitZero();}
 if(state.index>=state.ring.size())throw std::out_of_range("publication ring index");
 state.ring[state.index]=block;++state.produced;state.index=(state.index+1u)%500u;
}
