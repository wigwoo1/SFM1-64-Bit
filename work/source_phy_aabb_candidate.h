#pragma once
#include "source_phy_point_support_binding_candidate.h"
#include "source_physics_ray_preparation_candidate.h"
#include <limits>
// Original GetAABB1002A4F0/29F80, for the checked one-solid compact format.
// Geometry/metadata are owned; no original object pointers or DLL calls.
inline std::array<float,6>sourcePhyAabbCandidate(
    const SourcePhyCompactGeometryCandidate&geometry,
    const std::vector<SourcePhysicsSupportMetadataRecordCandidate>&metadata,
    const std::array<float,3>&origin,const std::array<float,3>&angles){
    if(geometry.solids.size()!=1||geometry.collisionTrees.size()!=1||geometry.solids[0].empty()||geometry.collisionTrees[0].empty())throw std::runtime_error("physics bounds require one checked solid/tree");
    const auto&solid=geometry.solids[0];if(!metadata.empty()&&metadata.size()!=solid.size())throw std::runtime_error("physics bounds metadata extent");
    for(float x:origin)if(!std::isfinite(x))throw std::runtime_error("nonfinite physics bounds origin");
    // Original01120 rotation is shared with point-ray preparation. No ray
    // arithmetic is needed here, so avoid fabricating a ray or length.
    std::array<uint32_t,3>bits{};std::memcpy(bits.data(),angles.data(),12);const auto words=angleBasisCandidate(bits);std::array<float,9>basis{};std::memcpy(basis.data(),words.data(),36);
    const std::array<float,9>rotation{basis[0],-basis[6],-basis[3],-basis[2],basis[8],basis[5],basis[1],-basis[7],-basis[4]};
    constexpr float metres=.02539999969303608f,units=39.370079040527344f;
    const std::array<float,3>translation{(origin[0]*metres)*units,(origin[1]*metres)*units,(-(origin[2]*metres))*(-units)};
    for(float x:translation)if(!std::isfinite(x))throw std::runtime_error("physics bounds translation overflow");
    const bool blocked=origin[0]!=0.f||origin[1]!=0.f||origin[2]!=0.f;
    std::array<float,6>bounds{std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),-std::numeric_limits<float>::max()};
    SourcePhysicsSupportCacheCandidate cache;cache.generation=1;
    const std::array<std::array<float,3>,6>directions{{{-1,0,0},{0,-1,0},{0,0,-1},{1,0,0},{0,1,0},{0,0,1}}};
    // The reader retains root-first/left-first leaf order, the recursion order
    // in original29F80. A single-leaf root writes components directly.
    const bool singleLeaf=geometry.collisionTrees[0].size()==1;
    for(size_t ordinal=0;ordinal<solid.size();++ordinal){
        const auto&convex=solid[ordinal];const auto*m=metadata.empty()?nullptr:&metadata[ordinal];
        auto context=sourcePhyPointSupportBindingCandidate(convex,uint32_t(ordinal),blocked?nullptr:m,rotation);
        for(unsigned row=0;row<3;++row)context.transform[row*4+3]=translation[row];
        if(blocked&&m){
            if(m->ledge!=ordinal)throw std::runtime_error("physics bounds metadata ledge");const uint8_t flags=m->payload[2];
            if(flags&2){uint16_t first=0,count=0;std::memcpy(&count,m->payload.data(),2);std::memcpy(&first,m->payload.data()+4,2);if(!count||uint64_t(first)+count>context.points.size())throw std::runtime_error("physics bounds contiguous span");context.mode=1;context.firstVertex=first;context.points=std::vector<std::array<float,3>>(context.points.begin()+first,context.points.begin()+first+count);}
            else if(flags&1){context.seeded=true;std::memcpy(context.seeds.data(),m->payload.data()+4,16);}
        }
        for(unsigned axis=0;axis<6;++axis){
            const auto point=sourcePhysicsSupportCandidate(context,directions[axis],cache).point;
            if(singleLeaf)bounds[axis]=point[axis%3];
            else for(unsigned j=0;j<3;++j){if(bounds[j]>point[j])bounds[j]=point[j];if(point[j]>bounds[3+j])bounds[3+j]=point[j];}
        }
    }
    for(unsigned j=0;j<3;++j)if(!std::isfinite(bounds[j])||!std::isfinite(bounds[3+j])||bounds[j]>bounds[3+j])throw std::runtime_error("invalid physics bounds result");return bounds;
}
