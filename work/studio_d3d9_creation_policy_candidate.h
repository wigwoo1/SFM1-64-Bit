#pragma once
#include "studio_d3d9_creation_flags_candidate.h"
#include "studio_d3d9_creation_attempts_candidate.h"
// Native output-local initialization precedes configuration/CPU callbacks.
template<class Services,class Backend>auto studioD3D9CreationPolicyCandidate(
 uint32_t incomingFlags,Services& services,Backend& backend){
 auto* output=backend.initialOutput();
 backend.configureFlags(studioD3D9CreationFlagsCandidate(incomingFlags,services));
 return studioD3D9CreationAttemptsWithOutputCandidate(backend,output);
}
