#pragma once
#include <cstdint>
// Scalar branch103568D0; supplied attributes retain native linked traversal order.
template<class Attributes,class ReadType,class ReadTarget,class Resolve,class IsElement,class Suppressed,class Eligible,class Mutate>
void registerRecoveredScalarReferencesCandidate(const Attributes&attributes,uint32_t context,bool removal,ReadType readType,ReadTarget readTarget,Resolve resolve,IsElement isElement,Suppressed suppressed,Eligible eligible,Mutate mutate){
 for(auto attribute:attributes){if(readType(attribute)!=1)continue;auto target=resolve(readTarget(attribute));
  if(target==0xffffffffu||!isElement(target)||suppressed(target)||!eligible(target,context))continue;
  mutate(target,attribute,removal);}
}
