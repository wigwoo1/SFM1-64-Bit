#pragma once
#include <cstdint>
// Logical100537a0 pass control. Ops owns native parameter info construction
// and each complete effect helper; this adapter does not implement their GPU work.
template<class Ops>void studioVertexLitPassesCandidate(uintptr_t shadow,Ops& ops){
 if(ops.cloakEnabled()&&!shadow&&ops.cloakOpaque())ops.skip(0);
 else ops.mainLighting();
 if(ops.cloakEnabled()){
  if(shadow)ops.cloak();
  else{float factor=ops.cloakFactor();if(factor>0.f&&factor<1.f)ops.cloak();else ops.skip(0);}
 }
 if(ops.emissiveEnabled()){
  if(shadow)ops.emissive();
  else{float strength=ops.emissiveStrength();if(strength>0.f)ops.emissive();else ops.skip(0);}
 }
 if(ops.fleshEnabled())ops.flesh();
}
