#pragma once
#include <array>
#include <cstdint>
// Original counter exit callback100f9b10 is a single RET; lifetime belongs to the external service.
inline void studioIndexAllocationCounterExitCandidate() noexcept {}
// Logical pointer-wide buffer object; not the original x86 layout.
struct StudioIndexBufferCreationCandidate{
 std::uintptr_t resource=0;
 std::array<std::uint32_t,7> fields{};
};
template<class Ops>void studioIndexBufferCreationCandidate(StudioIndexBufferCreationCandidate& state,std::uint32_t count,std::uint32_t managed,std::uint32_t dynamic,Ops& ops){
 state.resource=0;state.fields[0]=(count+1u)&~1u;state.fields[1]=0;
 auto flags=std::uint8_t(state.fields[2]);flags=std::uint8_t((flags&0xfau)|2u|((std::uint8_t(dynamic)&1u)<<2));state.fields[2]=(state.fields[2]&0xffffff00u)|flags;
 state.fields[3]=0xffffffffu;state.fields[4]=state.fields[5]=0;
 const std::uint32_t usage=8u|((flags&4u)?0x200u:0u)|(std::uint8_t(managed)?0x10u:0u);
 const std::uint32_t pool=(flags&4u)?0u:1u,bytes=state.fields[0]*2u;
 ops.flush();auto result=ops.create(ops.device(),bytes,usage,0x65u,pool,state.resource);
 if(result!=0)ops.logFailure(result);
 if(result==0x8876017cu||result==0x8007000eu){if(ops.device()){ops.flush();ops.cleanup(ops.device());}ops.flush();ops.create(ops.device(),bytes,usage,0x65u,pool,state.resource);}
 if(!(std::uint8_t(state.fields[2])&4u)){ops.ensureAllocationCounter();ops.addAllocationBytes(state.fields[0]*2u);}
}
