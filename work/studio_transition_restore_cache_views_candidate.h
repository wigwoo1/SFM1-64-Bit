#pragma once
#include "studio_restore_blend_candidate.h"
struct StudioTransitionRestoreDepthViewCandidate{
 std::uint32_t& state7;std::uint32_t& state23;std::uint32_t& mode124;std::uint8_t& dirty178;
 StudioTransitionRestoreDepthViewCandidate(StudioRestoreBlendStateCandidate& state,std::uint8_t& dirty):state7(state.f11c),state23(state.f120),mode124(state.f124),dirty178(dirty){}
};
struct StudioTransitionRestoreAlphaViewCandidate{
 std::uint8_t& enabled128;std::uint32_t& function12c;std::uint32_t& reference130;
 explicit StudioTransitionRestoreAlphaViewCandidate(StudioRestoreBlendStateCandidate& state):enabled128(state.f128),function12c(state.f12c),reference130(state.f130){}
};
struct StudioTransitionRestoreBlendViewCandidate{
 std::uint8_t& enabledFc;std::uint32_t& operation100;std::uint32_t& source104;std::uint32_t& destination108;
 explicit StudioTransitionRestoreBlendViewCandidate(StudioRestoreBlendStateCandidate& state):enabledFc(state.ffc),operation100(state.f100),source104(state.f104),destination108(state.f108){}
};
struct StudioTransitionRestoreSeparateBlendViewCandidate{
 std::uint8_t& enabled10c;std::uint32_t& operation110;std::uint32_t& source114;std::uint32_t& destination118;
 explicit StudioTransitionRestoreSeparateBlendViewCandidate(StudioRestoreBlendStateCandidate& state):enabled10c(state.f10c),operation110(state.f110),source114(state.f114),destination118(state.f118){}
};
