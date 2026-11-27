#pragma once

// Admission to engine trigger updates only. Does not enumerate overlaps,
// run trigger outputs, initialize/spawn actors or implement collision.
// Precondition: collision mode virtual is not the diagnostic mode 1.
template<class Host>
void sourceMapAdmitTriggerUpdateCandidate(Host& host) {
    if(!host.hasEdict() || host.isExcludedActor()) return;
    const auto flags=host.solidFlags();
    const bool trigger=(flags&8)!=0;
    const bool solid=host.solidType()!=0 && !(flags&4) && !trigger;
    if(!solid && !trigger) return;
    host.collisionMode();
    host.markForUpdate();
    if(solid) host.updateSolid();
    if(trigger) host.updateTrigger();
}
