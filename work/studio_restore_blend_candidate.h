#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>
#include "studio_restore_commit_candidate.h"
struct StudioRestoreBlendStateCandidate{
 std::uint8_t ffc=0,f10c=0,f128=0,f144=0;
 std::uint32_t f100=0,f104=0,f108=0,f110=0,f114=0,f118=0,f11c=0,f120=0,f124=0,f12c=0,f130=0;
 std::int16_t kind=0,cachedKind=0,cachedHandle=0;
 std::uint32_t descriptor=0;
 struct Sampler{std::uint8_t first=0,second=0,unused=0;};std::vector<Sampler> samplers;
 StudioRestoreCommitStateCandidate commit;
};
template<class Services>void studioRestoreBlendCandidate(StudioRestoreBlendStateCandidate& state,Services& services){
 state.ffc=0;state.f100=2;state.f104=state.f108=1;
 services.render(27,0);services.render(19,state.f100);services.render(20,state.f104);services.render(171,state.f108);
 state.f10c=0;state.f110=2;state.f114=state.f118=1;
 services.render(206,0);services.render(207,state.f110);services.render(208,state.f114);services.render(209,state.f118);
 state.f11c=1;state.f120=4;state.f124=0;
 services.render(7,1);services.render(23,state.f120);
 state.f128=0;state.f12c=7;state.f130=0;
 services.render(15,0);services.render(25,state.f12c);services.render(24,state.f130);
 const std::int32_t count=services.samplerCount();
 if(count>0&&static_cast<std::size_t>(count)>state.samplers.size())throw std::out_of_range("restore sampler state extent");
 for(std::int32_t stage=0;stage<count;++stage){const auto value=state.samplers[stage].first;
  services.sampler(static_cast<unsigned>(stage),11,value);
  if(static_cast<std::uint8_t>(services.specialSamplerEnabled())){const auto second=state.samplers[stage].second;
   services.sampler(static_cast<unsigned>(stage),8,0x31544547u+(second?0x03000000u:0u));}}
 const auto kind=state.kind;
 state.commit.dirty13c=state.commit.dirty13e=state.commit.dirty134=state.commit.dirty135=0;state.f144=0;
 if(!static_cast<std::uint8_t>(services.suppressed())){services.transition(kind,state.descriptor&0xffffffu,state.descriptor>>24);services.commit();state.cachedKind=kind;}
 services.vertexShader(-1);services.pixelShader(-1);state.cachedHandle=-1;
}
