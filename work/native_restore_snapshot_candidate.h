#pragma once
// Services expose typed staging handles; copying state must not transfer the
// source state's staging allocation ownership to the destination.
template<class Services>void nativeRestoreSnapshotCandidate(Services& services){
 const auto staging=services.staging();
 services.copyState();
 services.restoreStaging(staging);
}
