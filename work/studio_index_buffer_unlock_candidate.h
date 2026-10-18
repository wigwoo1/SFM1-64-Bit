#pragma once
#include <array>
#include <cstdint>
struct StudioIndexBufferUnlockStateCandidate{uintptr_t resource=0;uint32_t usedIndices=0;uint8_t flags=0;std::array<uint32_t,3> metadata{};};
struct StudioIndexBufferUnlockCommandCandidate{uint32_t opcode=0;uintptr_t resource=0;std::array<uint32_t,3> metadata{};uint32_t bytes=0;};
// Logical10005440. Command payload is typed/full-width, not native packed ABI.
// Callers serialize buffer state; resourceCOMcallback returns signedHRESULT.
template<class Enabled,class Emit,class Unlock,class Diagnostic>
void studioIndexBufferUnlockCandidate(StudioIndexBufferUnlockStateCandidate& state,uint32_t count,Enabled enabled,Emit emit,Unlock unlock,Diagnostic diagnostic){
 if(!(state.flags&1)||!state.resource)return;
 auto resource=state.resource;bool extended=(state.flags&4)!=0;
 if(enabled()){StudioIndexBufferUnlockCommandCandidate command;command.opcode=extended?31:19;command.resource=resource;if(extended){command.metadata=state.metadata;command.bytes=count*uint32_t(2);}emit(command);}
 else if(unlock(resource)<0)diagnostic(extended?0x420u:0x406u);
 state.usedIndices+=count;state.flags&=uint8_t(0xfe);
}
