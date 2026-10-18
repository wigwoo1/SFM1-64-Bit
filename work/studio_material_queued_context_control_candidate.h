#pragma once
#include <atomic>
#include "studio_material_mixed_queue_owner_candidate.h"
#include "studio_material_context_nesting_candidate.h"
// Recovered control subset, not the complete public render-context ABI.
class StudioMaterialQueuedContextControlCandidate {
 std::atomic<uint32_t> references{1};uint32_t nesting=0;
 uintptr_t immediate;StudioMaterialZeroArgumentCallCandidate::Method beginMethod,endMethod;
 StudioMaterialMixedQueueOwnerCandidate calls;
public:
 StudioMaterialQueuedContextControlCandidate(uintptr_t receiver,StudioMaterialZeroArgumentCallCandidate::Method begin,StudioMaterialZeroArgumentCallCandidate::Method end):immediate(receiver),beginMethod(begin),endMethod(end){}
 uint32_t addReference(){return references.fetch_add(1)+1;}
 template<class Destroy>uint32_t releaseReference(Destroy destroy){auto remaining=references.fetch_sub(1)-1;if(!remaining)destroy(this);return remaining;}
 uint32_t referenceCount()const{return references.load();}
 void begin(){const auto before=nesting;try{studioMaterialBeginCandidate(nesting,[&]{calls.enqueueZero(immediate,beginMethod);});}catch(...){nesting=before;throw;}}
 void end(){const auto before=nesting;try{studioMaterialEndCandidate(nesting,[&]{calls.enqueueZero(immediate,endMethod);});}catch(...){nesting=before;throw;}}
 void enqueue(StudioDecalQueuedCallCandidate* call){calls.enqueue(call);}
 StudioMaterialMixedQueueOwnerCandidate& queue(){return calls;}
 uint32_t nestingCount()const{return nesting;}
 void flush(){calls.drain();}
};

