#pragma once
#include "reference_registration_update.h"
#include "reference_registration_observers.h"
#include "reference_dirty_propagation.h"
#include "reference_update_queues.h"
// Element exposes state, flags, handle, cachePresent; caller adapters supply
// observer/reverse-reference order and clear owned cache lists.
template<class Observer,class Element,class Blocked,class Observers,class Notify,class Reverse,class Clear>
bool updateRecoveredReferenceLifecycleCandidate(Element& e,RecoveredReferenceUpdateQueuesCandidate& queues,uint32_t enabled,Blocked blocked,Observers observers,Notify notify,Reverse reverse,Clear clear){
 auto dirty=[&](uint32_t request){dirtyRecoveredReferenceGraphCandidate(&e,request,blocked,[&](Element*target){queues.dirty.append(target->handle);},[&](Element*target){queues.cache.append(target->handle);},reverse);};
 return updateRecoveredReferenceRegistrationCandidate(e.state,e.flags,enabled,[&](){notifyRecoveredReferenceRegistrationObserversCandidate<Observer>(e,blocked(),[&](auto visit){observers(e,visit);},notify);},[&](){queues.registration.append(e.handle);},dirty,[&](uint32_t offset){clear(e,offset);});
}
