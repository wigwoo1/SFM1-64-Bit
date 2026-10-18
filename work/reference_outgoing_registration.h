#pragma once
#include <cstdint>
// Whole103568D0 traversal. Attribute flags0800/1000 do not filter this routine.
// removal=true calls remove; false calls add. Target gates apply in both cases.
template<class Element,class Attributes,class Resolve,class Accept,class Registration,class Mutate>
void registerRecoveredOutgoingReferencesCandidate(Element& element,uint32_t context,bool removal,Attributes attributes,Resolve resolve,Accept accept,Registration registration,Mutate mutate){
 attributes(&element,[&](uint32_t token,uint16_t flags,const auto& handles){auto type=flags&31;if(type!=1&&type!=15)return;for(auto h:handles){auto* target=resolve(h);if(target&&accept(target)&&!registration(*target).suppressed()&&registration(*target).eligible(context))mutate(*target,token,removal);}});
}
