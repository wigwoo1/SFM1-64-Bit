#pragma once
#include "source_light_cache_new_record_anchor_candidate.h"
#include "source_leaf_ambient_interpolation_candidate.h"

// Explicit branch snapshot, not registered console defaults or observed live
// settings. Only the leaf-interpolation branch is implemented here.
struct SourceNewRecordLeafAmbientModeCandidate {
    int32_t radiosity;
    bool alternateAmbientPath;
    uint8_t engineState49,engineState4A;
};
struct SourceNewRecordLeafAmbientCandidate {
    std::array<float,3>queryPoint{},samplePoint{};
    uint32_t inputLeaf=0;
    SourceBspAmbientEncodingCandidate encoding;
    SourceLeafAmbientInterpolationCandidate::Cube cube{};
};
// Composition of engine101977B5/101977F2,10197923..101979DE,
// 101969A0..10196A0A and mode4 false branch1019323F..10193257.
// Crucial: EBX retains the ORIGINAL input leaf; ESI supplies the new record's
// sample position. Do not re-query a leaf at that sample point here.
// New record ONLY. No cache lookup/hit/reuse/refresh/lifetime policy.
// World and explicit ambient dataset must refer to the same immutable BSP.
template<class World>
SourceNewRecordLeafAmbientCandidate sourceNewRecordLeafAmbientCandidate(
    const std::array<float,3>&point,int32_t cacheCenter,
    const SourceNewRecordLeafAmbientModeCandidate&mode,World&world,
    const SourceBspLeafAmbientCandidate&ambient){
    if(mode.radiosity!=4||mode.alternateAmbientPath||mode.engineState49==1||mode.engineState4A==1)
        throw std::invalid_argument("new-record leaf ambient branch not selected");
    for(float value:point)if(!std::isfinite(value))throw std::invalid_argument("nonfinite new-record ambient point");
    const int32_t leaf=world.leafAt(point);
    if(leaf<0||size_t(leaf)>=ambient.leaves.size())throw std::out_of_range("new-record ambient input leaf");
    SourceNewRecordLeafAmbientCandidate result;result.queryPoint=point;result.inputLeaf=uint32_t(leaf);result.encoding=ambient.encoding;
    result.samplePoint=sourceLightCacheNewRecordAnchorCandidate(point,leaf,cacheCenter,world);
    result.cube=SourceLeafAmbientInterpolationCandidate::evaluate(ambient,result.inputLeaf,result.samplePoint);
    return result;
}
