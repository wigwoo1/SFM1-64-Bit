#pragma once
#include <cstdint>
struct StudioShutdownServicesCandidate {
 std::uintptr_t resource=0,cvar=0,resourceToken=0;
 std::uint32_t cvarHandle=0xffffffff;
 bool resourceRegistered=false,cvarRegistered=false;
};
template<class State,class Ops>void studioResourceShutdownCandidate(State& state,Ops& ops){
 if(!state.resource||!state.resourceRegistered)return;
 ops.resourceSlot9(state.resource,state.resourceToken);state.resourceRegistered=false;
}
template<class State,class Ops>void studioCvarShutdownCandidate(State& state,Ops& ops){
 if(!state.cvar||!state.cvarRegistered)return;
 ops.cvarSlot32(state.cvar,state.cvarHandle);
 ops.cvarSlot11(state.cvar,state.cvarHandle);
 state.cvarHandle=0xffffffff;state.cvarRegistered=false;
}

