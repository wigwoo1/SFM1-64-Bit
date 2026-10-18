#pragma once
#include "reference_cleanup_root_phases.h"
#include "reference_cleanup_iterator_phases.h"
// Entire recovered1035B920 phase order; callbacks may mutate scene state.
template<class Raw,class Registration,class Mark,class Dependencies,class Enumerate,class Resolve,class Register,class Update,class Notify,class Remove>
void recoveredReferenceCleanupPassCandidate(Raw raw,Registration registration,Mark mark,Dependencies dependencies,Enumerate enumerate,Resolve resolve,Register registerReferences,Update update,Notify notify,Remove remove){
 recoveredReferenceCleanupRootPhasesCandidate(raw,registration,mark,dependencies,resolve);
 recoveredReferenceCleanupIteratorPhasesCandidate(enumerate,resolve,registration,mark,registerReferences,update,notify,remove);
}
