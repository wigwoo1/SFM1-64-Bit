#pragma once
#include "engine_projection_inverse_candidate.h"
#include "engine_projection_sphere_candidate.h"
struct EngineProjectionMathOutputCandidate {
 EngineProjectionPlanesCandidate planes;
 float center[3];
 float radius;
};
// Math sequence of1010CF30 after cleanup/gates, before item enumeration.
// Explicit scratch represents caller storage. Original leaves it unwritten on
// inverse failure yet consumes it; caller must supply defined bytes here.
// This does not assert an original initialization policy or identity fallback.
// Input/shadow matrix copying preserves forward word-copy overlap semantics.
inline bool engineProjectionMathStageCandidate(const float* input,float* shadowMatrix,
 float* inverseScratch,EngineProjectionMathOutputCandidate& output){
 if(input!=shadowMatrix)for(int i=0;i<16;++i){uint32_t word;std::memcpy(&word,input+i,4);std::memcpy(shadowMatrix+i,&word,4);}
 bool inverted=engineProjectionInverseCandidate(shadowMatrix,inverseScratch);
 engineProjectionGeneratePlanesCandidate(inverseScratch,output.planes);
 engineProjectionSphereCandidate(inverseScratch,output.center,output.radius);
 return inverted; // Diagnostic only: both consumers have already run.
}
