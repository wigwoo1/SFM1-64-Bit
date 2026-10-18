#pragma once
#include <cstdint>
struct StudioDeviceManagerBaseShutdownCandidate {
 std::uintptr_t service54=0,service58=0,ownedTree=0;
};
// Host services retain their distinct ResourceSystem/Cvar/KeyValues ownership.
template<class State,class Ops> void studioDeviceManagerBaseDisconnectCandidate(State& state,Ops& ops){
 state.service54=0;state.service58=0;
 ops.resourceCleanup();ops.cvarCleanup();ops.interfaceCleanup();
 if(state.ownedTree){ops.deleteOwnedTree(state.ownedTree);state.ownedTree=0;}
}

