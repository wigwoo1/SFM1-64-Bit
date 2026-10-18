#pragma once
#include "reference_root_mark_state.h"
// Attribute enumeration must preserve native linked-list order; array handles ascend.
template<class Element,class State,class Attributes,class Resolve,class Accept>
void markRecoveredReferenceRootCandidate(Element* element,uint32_t enabled,uint32_t mode,uint32_t context,State state,Attributes attributes,Resolve resolve,Accept accept){
 if(!beginRecoveredReferenceRootMarkCandidate(state(element),enabled,context))return;
 attributes(element,[&](uint16_t flags,const auto& handles){
  if(mode>2 || (mode>=1&&(flags&0x1000)) || (mode==2&&!(flags&0x800)))return;
  const auto type=flags&31;if(type!=1&&type!=15)return;
  for(auto handle:handles){auto* target=resolve(handle);if(target&&accept(target))markRecoveredReferenceRootCandidate(target,enabled,mode,context,state,attributes,resolve,accept);}
 });
}
