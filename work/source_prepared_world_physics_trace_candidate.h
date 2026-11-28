#pragma once
#include "source_swept_hull_world_trace_candidate.h"
#include "source_physics_search_candidate.h"

// Translate the public world trace BEFORE the engine's world annotation.
// Surface names and worldSurfaceIndex are separate namespaces. The host must
// resolve a selected semantic surface to its native-width name identity;
// default ID0 already denotes the original default name. Do not encode the
// internal displacementHit marker or mesh ordinal as a production name pointer.
template<class SurfaceNameIdentity>
SourcePhysicsSearchTraceCandidate sourcePreparedWorldPhysicsTraceCandidate(
 const SourceSweptHullWorldTraceCandidate& input,SurfaceNameIdentity surfaceNameIdentity){
 SourcePhysicsSearchTraceCandidate output;
 const auto& state=input.state;const auto& trace=state.trace;
 output.start=input.startPosition;output.end=input.endPosition;
 for(size_t a=0;a<3;++a)output.plane.normal[a]=trace.plane[a];
 output.plane.distance=trace.plane[3];output.planeTags=trace.planeTags;
 output.fraction=trace.fraction;output.contents=trace.contents;
 output.displacementFlags=state.displacementFlags;
 output.allSolid=uint8_t(trace.allSolid);output.startSolid=uint8_t(trace.startSolid);
 output.fractionLeftSolid=trace.leftSolid;
 // Name identity is independent of properties and world-surface index. The
 // caller resolves every semantic kind, including the prepared default.
 output.surfaceId=surfaceNameIdentity(trace.surface);
 output.surfaceMetadata=uint32_t(state.surfaceProperty)|(uint32_t(state.surfaceFlags)<<16);
 output.worldSurface=input.worldSurfaceIndex;
 return output;
}
