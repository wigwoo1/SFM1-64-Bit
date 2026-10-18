#pragma once
#include "recovered_time_snap_candidate.h"
#include "recovered_float_rate_candidate.h"
#include <array>
#include <optional>
// Owned state corresponding to selected ifm1029A950 fields; no x86 pointer layout.
// Field names stay offset-based until owner/mode semantics are independently proved.
struct AnchoredDocumentTimeState {
 // Native constructor supplies a converted value; callers must supply observed state.
 explicit AnchoredDocumentTimeState(uint32_t anchor):anchor430(anchor){}
 uint32_t anchor430;
 int32_t saved684=0;
 int32_t mode47c=0;
 std::array<int32_t,4> modes480{};
 bool flag69c=false,flag228=false,flag2a8=false,flag6c0=false;
};
// Callbacks model unresolved engine slot29, monotonic engine tick conversion,
// and frameRate attribute lookup. Only selected branches invoke their callbacks.
template<class EngineGate,class EngineTicks,class FrameRate>
int32_t anchoredDocumentTimeCandidate(const AnchoredDocumentTimeState&s,
 EngineGate&&engineGate,EngineTicks&&engineTicks,FrameRate&&frameRate){
 const bool gate=engineGate();
 bool mode=s.mode47c!=0;
 for(auto value:s.modes480)mode=mode||value==2;
 if(gate&&mode){
  if(s.anchor430==0x80000000u)return s.saved684;
  const uint32_t word=uint32_t(engineTicks())-s.anchor430;
  int32_t result;std::memcpy(&result,&word,4);return result;
 }
 if(s.flag69c||s.flag228||s.flag2a8||!s.flag6c0)return s.saved684;
 const std::optional<float> rate=frameRate();
 return recoveredTimeSnapCandidate(s.saved684,recoveredFloatRateCandidate(rate.value_or(24.0f)));
}
