#pragma once
#include "attribute_mutation_gate.h"
#include "element_mutation_state.h"
#include "reference_owner_preparation.h"
#include "reference_update_queues.h"
template<class Node,class Key,class Next,class Blocked>
bool invalidateRecoveredIncomingAttributeCandidate(uint16_t& flags,int phase,uint8_t& ownerState,uint32_t ownerHandle,uintptr_t attribute,Node*& source,Node*& destination,RecoveredReferenceUpdateQueuesCandidate& queues,Key key,Next next,Blocked blocked){
 return beginAttributeMutation(flags,phase,[&](){prepareRecoveredReferenceOwnerCandidate(source,destination,attribute,blocked(),key,next,[&](){queues.cache.append(ownerHandle);});},[&](){markElementMutation(ownerState,ownerHandle,[&](uint32_t h){queues.registration.append(h);});});
}
