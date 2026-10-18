#pragma once
#include <array>
#include "studio_constant_buffer_replace_candidate.h"
#include "studio_constant_storage_initialize_candidate.h"
struct StudioConstantStorageOwnerCandidate{
 std::array<void*,6> primary{},staging{};
};
// Recovered ordered helper policy. Initialization borrows this same storage.
// This low-level sequence requires successful allocations; owned rollback and
// complete original-helper parity remain separate integration requirements.
template<bool CheckAllocation,class Allocation,class Initialization,class Tail>
bool studioConstantStorageRestoreSequenceCandidate(StudioConstantStorageOwnerCandidate& owner,
 Allocation& allocation,Initialization& initialization,Tail& tail){
 for(unsigned group=0;group<2;++group){
  for(unsigned kind=0;kind<3;++kind){
   const unsigned id=group*3+kind;const uint32_t stride=kind==1?4u:16u;
   studioConstantBufferReplaceCandidate(owner.primary[id],id,stride,allocation);
   if constexpr(CheckAllocation)if(!owner.primary[id])return false;
   studioConstantBufferReplaceCandidate(owner.staging[id],id,stride,allocation);
   if constexpr(CheckAllocation)if(!owner.staging[id])return false;
  }
  studioConstantStorageInitializeGroupCandidate(group!=0,initialization);
 }
 tail.restoreVertexDefaults(0x40000000);
 tail.unbindShaders();
 return true;
}

template<class Allocation,class Initialization,class Tail>
void studioConstantStorageRestoreCandidate(StudioConstantStorageOwnerCandidate& owner,
 Allocation& allocation,Initialization& initialization,Tail& tail){
 studioConstantStorageRestoreSequenceCandidate<false>(owner,allocation,initialization,tail);
}

// Owned-runtime failure policy, not a claim about original null-allocation behavior.
// A false result leaves a partial restore: earlier replacements/publications remain,
// the failed slot is null, and later slots retain their previous allocations.
// Rendering must remain disabled until a subsequent complete restore succeeds.
// Allocation services must return nonnull owned storage even for zero-byte requests.
template<class Allocation,class Initialization,class Tail>
bool studioConstantStorageRestoreCheckedCandidate(StudioConstantStorageOwnerCandidate& owner,
 Allocation& allocation,Initialization& initialization,Tail& tail){
 return studioConstantStorageRestoreSequenceCandidate<true>(owner,allocation,initialization,tail);
}
