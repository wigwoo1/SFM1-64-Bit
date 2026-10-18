#pragma once
#include <cstdint>
// Array branch103568D0 visits every entry in ascending index order, including duplicates.
template<class Targets,class Resolve,class IsElement,class Suppressed,class Eligible,class Mutate>
void registerRecoveredArrayReferencesCandidate(uint32_t attribute,const Targets&targets,uint32_t context,bool removal,Resolve resolve,IsElement isElement,Suppressed suppressed,Eligible eligible,Mutate mutate){
 for(auto handle:targets){auto target=resolve(handle);
  if(target==0xffffffffu||!isElement(target)||suppressed(target)||!eligible(target,context))continue;
  mutate(target,attribute,removal);}
}
