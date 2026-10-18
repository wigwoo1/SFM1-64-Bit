#pragma once
#include <cstdint>
//10365F90 internal helper, invoked after change/permission guard.
// Callbacks handle invalid targets; no no-op suppression at this level.
template<class Remove,class Add>
void assignRecoveredScalarReferenceStorageCandidate(uint32_t&storage,const uint32_t&incoming,Remove remove,Add add){
 remove(storage);storage=incoming;add(incoming);
}
