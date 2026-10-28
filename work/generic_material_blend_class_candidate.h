#pragma once
#include <cstdint>
// stdshader10064D30. Ops supplies valid live lists/variables/resources. Field
// reads are side-effect free; texture and textureAlpha are explicit callbacks.
template<class Ops>inline bool genericTextureAlphaCandidate(int32_t index,uint32_t alphaArgument,Ops&ops){
 if(index<0)return false;
 const auto list=ops.currentParameters();
 if((ops.typeByte(list,uint32_t(index))&15u)!=3)return false;
 if(uint8_t(alphaArgument)){
  const uint32_t flags=ops.primaryFlags(list);
  if(flags&0x1000000u)return false;
  if((flags&0x40u)&&!(ops.secondaryFlags(list)&0x400000u))return false;
  if(flags&0x100000u)return false;
  if(!(flags&0x200100u))return false;
 }
 const auto resource=ops.texture(ops.variable(list,uint32_t(index)));
 return uint8_t(ops.textureAlpha(resource))!=0;
}
// stdshader10002D20 +10064DD0. Result is the observed0..3 class; no GPU state
// mapping or resource ownership is inferred. Reads after callbacks stay live.
template<class Ops>inline uint32_t genericMaterialBlendClassCandidate(int32_t first,uint32_t alphaArgument,int32_t second,Ops&ops){
 bool alpha=(ops.snapshotWord()&1u)!=0;
 if(!alpha){
  if(ops.primaryFlags(ops.currentParameters())&0x20u)alpha=true;
  else if(genericTextureAlphaCandidate(first,alphaArgument,ops)&&!(ops.primaryFlags(ops.currentParameters())&0x100u))alpha=true;
 }
 if(second!=-1&&!alpha)alpha=genericTextureAlphaCandidate(second,alphaArgument,ops);
 return ((ops.primaryFlags(ops.currentParameters())&0x80u)?2u:0u)+uint32_t(alpha);
}
