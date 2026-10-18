#pragma once
// 1029aae0 protocol; mode scope ABI/time conversion/type token remain explicit
// adapter contracts. Type eligibility is checked again after inner restoration.
template<class Adapter>
inline auto resolveEditorMapFallbackCandidate(Adapter& a)->decltype(a.resolvePrimary()) {
    using Pointer=decltype(a.resolvePrimary());
    if(!a.resolvePrimary())return Pointer{};
    auto outer=a.beginModeScope();
    auto* primary=a.resolvePrimary();
    auto time=a.currentTime();
    auto inner=a.beginModeScope();
    Pointer candidate{};
    if(primary) {
        if(auto* track=a.track(primary)) {
            auto mapped=a.mapTime(primary,time,0);
            candidate=a.findClip(track,mapped);
            if(candidate && !a.eligible(candidate))candidate=nullptr;
        }
    }
    a.restoreModeScope(inner);
    if(candidate && !a.eligible(candidate))candidate=nullptr;
    a.restoreModeScope(outer);
    return candidate;
}
