#pragma once
#include "native_restore_snapshot_candidate.h"
template<class Services>void nativeRestoreSnapshotFinishCandidate(Services& services){
 nativeRestoreSnapshotCandidate(services);
 services.clearRestoreForce();
 services.completeRestore(true);
}
