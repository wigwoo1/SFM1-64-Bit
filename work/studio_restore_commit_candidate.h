#pragma once
#include <cstdint>
// Live fields; nested callbacks may change later branches and cached values.
struct StudioRestoreCommitStateCandidate{
 std::uint8_t dirty134=0,dirty135=0;std::uint32_t value138=0;
 std::uint8_t dirty13c=0,enabled13d=0,dirty13e=0,enabled13f=0;
 std::uint32_t cached140=0;
};
template<class Services>void studioRestoreCommitCandidate(StudioRestoreCommitStateCandidate& state,Services& services){
 if(state.dirty134)services.apply134(3);
 if(state.dirty135){services.apply135(1);const auto value=state.value138;services.submit(14,value);}
 if(state.dirty13c){const auto value=(state.cached140&~std::uint32_t{8})|(state.enabled13d?8u:0u);
  if(value!=state.cached140){state.cached140=value;services.submit(168,value);}}
 if(state.dirty13e){const auto value=(state.cached140&~std::uint32_t{7})|(state.enabled13f?7u:0u);
  if(value!=state.cached140){state.cached140=value;services.submit(168,value);}}
}
