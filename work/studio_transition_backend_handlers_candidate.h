#pragma once
#include <array>
#include <cstdint>
struct StudioTransitionState72Candidate{
 std::array<std::uint32_t,16> values{};
 std::uint32_t value40=0;std::uint8_t flags44=0,flags45=0,reserved46=0,reserved47=0;
};
static_assert(sizeof(StudioTransitionState72Candidate)==72);
template<class Services>void studioTransitionStagesCandidate(StudioTransitionState72Candidate* state,Services& services){
 const std::int32_t count=services.stageCount();
 for(std::int32_t index=0;index<count;++index)services.applyStage(state,index);
}
template<class Services>void studioTransitionFlag44Candidate(const StudioTransitionState72Candidate& state,Services& services){
 const unsigned enabled=(state.flags44>>4)&1;services.applyFlag44(enabled);
}
template<class Services>void studioTransitionFlag45Candidate(const StudioTransitionState72Candidate& state,Services& services){
 const unsigned enabled=(state.flags45>>3)&1;services.applyFlag45(enabled);
}
template<class Services>void studioTransitionCombinedFlagsCandidate(const StudioTransitionState72Candidate& state,Services& services){
 const unsigned first=state.flags44&1,second=state.flags44>>7,third=(state.flags45>>2)&1;
 const auto value=state.value40;services.applyCombined(value,first,second,third);
}
