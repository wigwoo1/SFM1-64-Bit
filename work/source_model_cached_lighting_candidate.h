#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

// Semantic owned data; no packed original x86 layout or borrowed light pointers.
// Identity tokens keep the full native width. Resource lifetime is the caller's.
struct SourceModelLightingStateCandidate {
    std::array<float,18> ambient{};
    std::vector<uintptr_t> lights;
};
struct SourceModelInstanceLightingCandidate {
    bool active=false;
    SourceModelLightingStateCandidate cached;
};
inline const SourceModelInstanceLightingCandidate* sourceModelInstanceLightingCandidate(
    uint16_t instance,const std::vector<SourceModelInstanceLightingCandidate>&records){
    // Original100D5804: FFFF means no record; otherwise index*108h at table+0Ch.
    if(instance==0xffff)return nullptr;
    if(instance>=records.size())throw std::runtime_error("model lighting instance index outside table");
    return &records[instance];
}
inline void sourceModelLightingStateCheckCandidate(const SourceModelLightingStateCandidate&s){
    if(s.lights.size()>4)throw std::runtime_error("model lighting list exceeds four lights");
    for(float x:s.ambient)if(!std::isfinite(x))throw std::runtime_error("nonfinite model ambient cube");
}
// Original100D2370: ordered identity merge. A full four-entry list skips a
// candidate completely. Duplicates also skip without querying capabilities.
// Capacity may be queried twice and may change between those calls.
template<class Services>
void sourceModelMergeLightingCandidate(SourceModelLightingStateCandidate&destination,
    const SourceModelLightingStateCandidate&source,const std::array<float,3>&point,Services&services){
    sourceModelLightingStateCheckCandidate(destination);sourceModelLightingStateCheckCandidate(source);
    for(uintptr_t light:source.lights){
        if(destination.lights.size()>=4)continue;
        bool duplicate=false;for(uintptr_t existing:destination.lights)if(existing==light){duplicate=true;break;}
        if(duplicate)continue;
        const int32_t configured=services.configuredLimit();
        const int32_t capacity=services.hardwareLimit();
        const int32_t effective=capacity<configured?services.hardwareLimit():services.configuredLimit();
        if(static_cast<int32_t>(destination.lights.size())<effective)destination.lights.push_back(light);
        else services.overflow(light,point,destination,false);
    }
}
// Original100D58C0..100D5A82, cached-handle post-query path only. The draw cube
// is written BEFORE list merge/overflow. Scratch ambient starts at zero in the
// caller (original100D5693..571B); do not silently copy the cached cube into it.
template<class Services>
bool sourceModelCachedLightingCandidate(bool enabled,const SourceModelInstanceLightingCandidate*record,
    bool outputEnabled,const SourceModelLightingStateCandidate&current,const std::array<float,3>&point,
    std::array<float,18>&drawAmbient,SourceModelLightingStateCandidate&scratch,Services&services){
    if(!enabled||!record||!outputEnabled||!record->active)return false;
    sourceModelLightingStateCheckCandidate(current);sourceModelLightingStateCheckCandidate(record->cached);
    for(float x:point)if(!std::isfinite(x))throw std::runtime_error("nonfinite cached lighting point");
    auto combined=drawAmbient;
    for(size_t i=0;i<combined.size();++i){combined[i]=current.ambient[i]+record->cached.ambient[i];if(!std::isfinite(combined[i]))throw std::runtime_error("cached ambient addition overflow");}
    drawAmbient=combined;
    scratch.lights=record->cached.lights; // Original100D0F20 copies only count/list.
    sourceModelMergeLightingCandidate(scratch,current,point,services);return true;
}
