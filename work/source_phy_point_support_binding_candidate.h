#pragma once
#include "source_physics_support_metadata_candidate.h"
// Original2A120 point-shape transform, composed with owned metadata, packed
// preparation and support policy. rotation is original01120's three columns;
// point rays have zero extents, so all three translation fields are +zero.
inline SourcePhysicsSupportContextCandidate sourcePhyPointSupportBindingCandidate(
    const SourcePhyConvexGeometryCandidate&convex,uint32_t ordinal,
    const SourcePhysicsSupportMetadataRecordCandidate*metadata,
    const std::array<float,9>&rotation){
    if(convex.rawTriangles.empty()||convex.rawPoints.empty())throw std::runtime_error("point support needs owned raw PHY data");
    if(metadata&&metadata->ledge!=ordinal)throw std::runtime_error("point support metadata ledge mismatch");
    for(float x:rotation)if(!std::isfinite(x))throw std::runtime_error("nonfinite point support rotation");
    SourcePhysicsSupportContextCandidate context;context.inverseBasis=rotation;
    constexpr float units=39.370079040527344f;
    for(unsigned i=0;i<3;++i){context.transform[i]=rotation[i]*units;context.transform[4+i]=rotation[6+i]*units;context.transform[8+i]=rotation[3+i]*(-units);}
    for(float x:context.transform)if(!std::isfinite(x))throw std::runtime_error("point support transform overflow");
    SourcePhysicsSupportMetadataCandidate preparation;
    if(metadata){preparation.present=true;preparation.flags=metadata->payload[2];std::memcpy(&preparation.count,metadata->payload.data(),2);std::memcpy(&preparation.firstVertex,metadata->payload.data()+4,2);if(preparation.flags&4){const size_t bytes=metadata->payload[3];if(bytes>14)throw std::runtime_error("point support run extent");preparation.runs.assign(metadata->payload.begin()+6,metadata->payload.begin()+6+bytes);}}
    if(preparation.present&&(preparation.flags&6)){
        std::vector<std::array<float,4>>raw;raw.reserve(convex.rawPoints.size());for(const auto&bits:convex.rawPoints){std::array<float,4>p;std::memcpy(p.data(),bits.data(),16);raw.push_back(p);}
        SourcePhysicsPackedSupportCandidate packed;packed.storage.resize((size_t(preparation.count)+3)/4*4);
        if(!sourcePhysicsPreparePackedSupportCandidate(preparation,false,raw,context.transform,packed))throw std::runtime_error("eligible point support cache declined");context.mode=2;context.points=std::move(packed.storage);return context;
    }
    context.mode=0;context.triangles=convex.rawTriangles;context.points.reserve(convex.rawPoints.size());for(const auto&bits:convex.rawPoints){std::array<float,3>p;std::memcpy(p.data(),bits.data(),12);context.points.push_back(p);}
    if(metadata&&(metadata->payload[2]&1)){context.seeded=true;std::memcpy(context.seeds.data(),metadata->payload.data()+4,16);}return context;
}
