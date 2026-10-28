#pragma once
#include <cstdint>
struct ShaderApiMaterialBindingStateCandidate {uintptr_t material=0;};
// ShaderApi030 slot83 (10013F30), with live callback semantics. Method7's
// predicate and method10's compared low32bit key deliberately have no inferred
// semantic names. Matching keys can retain a DIFFERENT previous material ID.
// Ops owns valid live identities/services. This adds no reference ownership,
// material-value snapshot, queue/thread safety or renderer admission.
template<class Ops>inline void shaderApiMaterialBindingCandidate(ShaderApiMaterialBindingStateCandidate&state,uintptr_t desired,Ops&ops){
 const auto previous=state.material;
 const auto replace=[&]{ops.flush();state.material=desired;};
 if(previous&&desired&&ops.predicate(previous)&&ops.predicate(desired)){
  const uint32_t oldKey=ops.key(state.material);
  const uint32_t newKey=ops.key(desired);
  if(oldKey!=newKey)replace();
  return;
 }
 const auto current=state.material;
 if(current!=desired){replace();return;}
 if(current&&ops.predicate(current)){replace();return;}
 if(desired&&ops.predicate(desired))replace();
}
