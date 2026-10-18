#pragma once
#include <array>
#include "studio_shader_runtime_pass_candidate.h"
struct StudioShaderSnapshotEntryCandidate {
 uint32_t passCount=0;std::array<uint16_t,4> handles{};
 // Remaining native44-byte entry fields require separate ownership recovery.
};
//100375f0. Capacity precondition: live entry.passCount<4 when capture returns.
template<class Ops>void studioShaderSnapshotCaptureCandidate(StudioShaderRuntimePassStateCandidate& state,Ops& ops){
 if(ops.hardwareLevel()>=92){ops.sampler21(15,1);ops.sampler20(15,1);}
 auto& entry=ops.entry(state.snapshot); // Retained before capture callback.
 auto handle=ops.capture();
 entry.handles[entry.passCount]=uint16_t(handle);entry.passCount+=uint32_t(1);
}
