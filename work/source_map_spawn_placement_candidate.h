#pragma once

// Clean, nonnull selected entity only. Host owns collision traces, actual
// transform/velocity setters, notifications and command/view representation.
template<class Host>
auto sourceMapPlaceSpawnCandidate(Host& host) {
    auto spawnEntity=host.selectSpawn();
    const auto maximum=host.bounds().maximum;
    const auto minimum=host.bounds().minimum;
    const auto start=host.spawnOrigin(spawnEntity);
    auto end=start; end[2]-=500.f;
    auto placed=host.trace(start,end,minimum,maximum,0x200400bu,0);
    placed[2]+=1.f;
    host.setLocalOrigin(placed);
    host.zeroVelocity();
    host.setLocalAngles(host.spawnAngles(spawnEntity));
    host.resetVectorA(); host.resetVectorB();
    host.snapEyeAngles(host.spawnAngles(spawnEntity));
    return spawnEntity;
}
