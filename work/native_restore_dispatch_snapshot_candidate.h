#pragma once
#include "native_restore_callback_lists_candidate.h"
#include "native_restore_snapshot_candidate.h"
// Capture destination staging ownership after all callbacks: callbacks may
// replace these allocations or update the state that the snapshot publishes.
template<class First,class Second,class Gate,class Snapshot,class... Arguments>
void nativeRestoreDispatchSnapshotCandidate(First& first,std::uint8_t& firstFlags,
 Second& second,std::uint8_t& secondFlags,Gate&& gate,Snapshot& snapshot,Arguments... arguments){
 nativeRestoreCallbackListsCandidate(first,firstFlags,second,secondFlags,gate,arguments...);
 nativeRestoreSnapshotCandidate(snapshot);
}
