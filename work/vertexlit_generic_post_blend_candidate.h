#pragma once
#include <array>
#include <cstdint>
// Material preparation after blend classification, for the installed
// VertexLitGeneric route only (vertexLit=1, flashlightMode=0).
// flashlight is the router's captured decision BEFORE blend callbacks.
// Ops reads the retained input list, NOT the shared current-parameters global.
// Order matches original locals 10055553..100557B0. No GPU submission,
// cached-context reuse, ownership, or other shader route is implemented here.
template<class Ops>inline std::array<uint32_t,15> vertexlitGenericPostBlendCandidate(uint32_t blend,bool flashlight,Ops&ops){
 const bool baseMask=!flashlight&&(ops.primaryFlags()&0x40u)!=0;
 const bool maskTexture=baseMask&&ops.textureDefined(0x148);
 const auto mode=ops.integer(0x17c);
 return {blend,uint32_t(blend!=1&&blend!=3&&!ops.initialAlphaTest()&&!flashlight),uint32_t(!flashlight&&ops.textureDefined(0x24)),uint32_t(ops.integer(0x138)!=1),0u,0u,
  uint32_t(mode!=0),mode<0?0u:mode>2?2u:uint32_t(mode),
  uint32_t(ops.integer(0xd4)!=0&&mode==0),
  uint32_t(ops.integer(0xd8)!=0&&mode==0),
  uint32_t(ops.integer(0xdc)!=0),uint32_t(baseMask),
  uint32_t(ops.scalar(0x9c)!=0.0f),uint32_t(!baseMask&&!maskTexture&&ops.scalar(0x160)>0.0f),uint32_t(maskTexture)};
}
