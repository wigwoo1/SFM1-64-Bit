#pragma once
#include <array>
#include <cstdint>
struct StudioVertexBufferUnlockStateCandidate{uintptr_t resource=0;uint32_t usedBytes=0,stride=0;uint8_t flags=0;std::array<uint32_t,3> metadata{};};
struct StudioVertexBufferUnlockCommandCandidate{uint32_t opcode=0;uintptr_t resource=0;std::array<uint32_t,3> metadata{};uint32_t bytes=0;};
// Logical10005a60. Positive stride and nonnegative representable signed
// cursor+stride-1 required. Typed commands do not establish packed native ABI.
template<class Enabled,class Emit,class Unlock,class Diagnostic>
void studioVertexBufferUnlockCandidate(StudioVertexBufferUnlockStateCandidate& state,uint32_t count,Enabled enabled,Emit emit,Unlock unlock,Diagnostic diagnostic){
 if(!(state.flags&2)||!state.resource)return;
 auto resource=state.resource;uint32_t aligned=((state.usedBytes+state.stride-1)/state.stride)*state.stride,bytes=count*state.stride;bool extended=(state.flags&1)!=0;
 if(enabled()){StudioVertexBufferUnlockCommandCandidate command;command.opcode=extended?29:18;command.resource=resource;if(extended){command.metadata=state.metadata;command.bytes=bytes;}emit(command);}
 else if(unlock(resource)<0)diagnostic(extended?0x3f4u:0x3dau);
 state.usedBytes=aligned+bytes;state.flags&=uint8_t(0xfd);
}
