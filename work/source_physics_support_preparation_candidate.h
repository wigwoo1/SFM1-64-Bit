#pragma once
#include "source_physics_support_candidate.h"
struct SourcePhysicsSupportMetadataCandidate {
    bool present=false;
    uint8_t flags=0;
    uint16_t count=0,firstVertex=0;
    std::vector<uint8_t>runs;
};
struct SourcePhysicsPackedSupportCandidate {
    uint32_t groups=0;
    // Four consecutive point entries correspond to the four original lanes.
    // Storage beyond the active groups is retained as opaque caller data.
    std::vector<std::array<float,3>>storage;
};
// Complete original26A10/266F0 preparation decision and finite-result geometry.
// Matrix is original18h's three rows. A8DC0 rotates the groups without adding
// the translation column; those three unused values may be nonfinite.
inline bool sourcePhysicsPreparePackedSupportCandidate(
    const SourcePhysicsSupportMetadataCandidate&metadata,bool blocked,
    const std::vector<std::array<float,4>>&points,const std::array<float,12>&matrix,
    SourcePhysicsPackedSupportCandidate&cache){
    if(!metadata.present||!(metadata.flags&6)||blocked)return false;
    if(!metadata.count||metadata.count>32768||metadata.firstVertex>=points.size())throw std::runtime_error("invalid packed support metadata extent");
    const size_t groups=(size_t(metadata.count)+3)/4,lanes=groups*4;
    if(cache.storage.size()<lanes)throw std::runtime_error("insufficient packed support storage");
    auto finite=[](float x){if(!std::isfinite(x))throw std::runtime_error("nonfinite support preparation arithmetic");return x;};
    for(unsigned row=0;row<3;++row)for(unsigned column=0;column<3;++column)finite(matrix[row*4+column]);
    std::vector<size_t>indices;indices.reserve(lanes);size_t cursor=metadata.firstVertex;
    if(metadata.flags&4){
        if(metadata.runs.empty()||metadata.runs.size()>14||metadata.runs.size()%2==0)throw std::runtime_error("invalid packed support runs");
        size_t remaining=metadata.runs[0],run=1;
        for(size_t i=0;i<metadata.count;++i){
            if(!remaining){if(run+1>=metadata.runs.size())throw std::runtime_error("truncated packed support run");cursor+=metadata.runs[run];remaining=metadata.runs[run+1];run+=2;if(!remaining)throw std::runtime_error("empty packed support run");}
            if(cursor>=points.size())throw std::runtime_error("packed support vertex outside input");indices.push_back(cursor++);--remaining;
        }
        if(remaining||run!=metadata.runs.size())throw std::runtime_error("packed support run count mismatch");
    }else{
        if(uint64_t(metadata.firstVertex)+metadata.count>points.size())throw std::runtime_error("packed support span outside input");
        for(size_t i=0;i<metadata.count;++i)indices.push_back(size_t(metadata.firstVertex)+i);
    }
    while(indices.size()<lanes)indices.push_back(indices.back());
    auto result=cache;result.groups=uint32_t(groups);
    for(size_t lane=0;lane<lanes;++lane){
        const auto&p=points[indices[lane]];for(unsigned component=0;component<3;++component)finite(p[component]);
        // Retain original vector operation grouping and signs. Padding words
        // and the translation column do not participate in the arithmetic.
        for(unsigned row=0;row<3;++row)result.storage[lane][row]=finite((p[0]*matrix[row*4]+p[1]*matrix[row*4+1])+p[2]*matrix[row*4+2]);
    }
    cache=std::move(result);return true;
}
