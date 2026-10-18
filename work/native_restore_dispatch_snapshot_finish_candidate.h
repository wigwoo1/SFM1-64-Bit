#pragma once
#include "native_restore_callback_lists_candidate.h"
#include "native_restore_snapshot_finish_candidate.h"
template<class First,class Second,class Gate,class Snapshot,class... Arguments>
void nativeRestoreDispatchSnapshotFinishCandidate(First& first,std::uint8_t& firstFlags,
 Second& second,std::uint8_t& secondFlags,Gate&& gate,Snapshot& snapshot,Arguments... arguments){
 nativeRestoreCallbackListsCandidate(first,firstFlags,second,secondFlags,gate,arguments...);
 nativeRestoreSnapshotFinishCandidate(snapshot);
}
