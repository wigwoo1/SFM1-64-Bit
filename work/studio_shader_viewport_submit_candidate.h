#pragma once
#include <array>
#include <cstdint>
using StudioShaderViewportWordsCandidate=std::array<uint32_t,6>;
// Nativecallback10019910 ignores supplied commandstate and uses staticstate.
// Ops supplies that owner. Raw words preserve viewport depth float bit patterns.
template<class Ops>void studioShaderViewportSubmitCandidate(const StudioShaderViewportWordsCandidate& live,StudioShaderViewportWordsCandidate& cache,uint32_t force,Ops& ops){
 if(!uint8_t(force)&&live==cache)return;
 if(!live[2]||!live[3])return;
 if(ops.queueEnabled())ops.queueViewport(live);else ops.directViewport(live);
 cache=live;
}
