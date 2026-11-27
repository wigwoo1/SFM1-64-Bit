#pragma once

// Service policy for installed valid team indices 0..3. Lookup, lifetime,
// runtime type checks and actual placement belong to the host.
template<class Host>
auto sourceMapSelectSpawnCandidate(Host& host) {
    auto selected=host.recentSpawn();
    const bool teamSpawn=host.team()==2 || host.team()==3;
    if(teamSpawn) {
        if(host.chooseTeamSpawn(selected)) host.cacheSpawn(selected);
        host.storeTypedSpawn(selected);
    } else selected=host.worldEntity();
    if(!selected) { host.warnMissingSpawn(teamSpawn); selected=host.worldEntity(); }
    return selected;
}
