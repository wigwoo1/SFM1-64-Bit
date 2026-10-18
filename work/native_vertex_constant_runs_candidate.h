#pragma once
#include "studio_vertex_constant_runs_candidate.h"
// Reuses the typed float submission service and its queue encoding/refill policy.
// Lifecycle supplies live current/cache pointers after device callbacks.
template<class FloatSubmission,class Lifecycle>
struct NativeVertexConstantRunsServicesCandidateT{
 FloatSubmission& submission;Lifecycle& lifecycle;
 float* current(){return lifecycle.current();}float* cache(){return lifecycle.cache();}
 void submit(uint32_t start,uint32_t count,const float* source){
  submission.submitFloat(true,start,count,reinterpret_cast<uint32_t*>(const_cast<float*>(source)));
 }
};
