#pragma once
#include "studio_decal_queued_call_candidate.h"
// material1000F710: append receives count+1, then original count is restored.
// Append stores the pointer without another reference (10005320). The caller's
// initial reference remains associated with the queue entry. Non-null precondition.
// x64 exception policy: failed append restores the caller reference before rethrow.
template<class Append,class Free>
inline void studioMaterialExternalEnqueueCandidate(StudioDecalQueuedCallCandidate* call,Append append,Free freeObject){
 call->addReference();try{append(call);}catch(...){call->releaseReference(freeObject);throw;}call->releaseReference(freeObject);
}

