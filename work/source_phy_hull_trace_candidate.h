#pragma once
#include "source_phy_point_support_binding_candidate.h"
#include "source_physics_hull_ray_preparation_candidate.h"
#include "source_physics_leaf_candidate.h"
struct SourcePhysicsHullTraceStatisticsCandidate {
    std::array<uint32_t,5> outcomes{};
    uint32_t packedSearches=0,walkSearches=0;
};
// File-to-result composition for the observed one-solid compact PHY format.
// Reuses accepted fixed-shape support, traversal/search and leaf policies with
// verified moving-box preparation. No original DLL or captured search state.
class SourcePhyHullTraceCandidate {
    SourcePhyCompactGeometryCandidate geometry;
    std::vector<SourcePhysicsSupportMetadataRecordCandidate> metadata;
public:
    explicit SourcePhyHullTraceCandidate(const std::vector<uint8_t>& bytes):geometry(bytes,true){
        if(geometry.solids.size()!=1||geometry.collisionTrees.size()!=1)throw std::runtime_error("hull trace currently requires one PHY solid");
        sourcePhysicsValidateTreeCandidate(geometry.collisionTrees[0]);
        std::vector<bool> seen(geometry.solids[0].size(),false);
        for(const auto& node:geometry.collisionTrees[0])if(!node.raw[0]){if(node.ledge>=seen.size()||seen[node.ledge])throw std::runtime_error("hull trace ledge mapping");seen[node.ledge]=true;}
        for(bool present:seen)if(!present)throw std::runtime_error("hull trace unreachable ledge");
        metadata=sourcePhysicsSupportMetadataTableCandidate(geometry.solids[0]);
    }
    SourcePhysicsSearchTraceCandidate traceHull(const SourceServerHullRayCandidate& ray,
        const std::array<float,3>& origin,const std::array<float,3>& angles,uint32_t mask,
        SourcePhysicsHullTraceStatisticsCandidate* statistics=nullptr)const{
        return traceHullWithContents(ray,origin,angles,mask,[](uint32_t){return 1u;},statistics);
    }
    // Fresh scratch per call, as in the observed full TraceBox caller. Callback
    // side effects belong to the caller; geometry and statistics stay separate.
    template<class Contents>
    SourcePhysicsSearchTraceCandidate traceHullWithContents(const SourceServerHullRayCandidate& ray,
        const std::array<float,3>& origin,const std::array<float,3>& angles,uint32_t mask,
        Contents&& contentsForGameData,SourcePhysicsHullTraceStatisticsCandidate* statistics=nullptr)const{
        const auto prepared=sourcePhysicsHullRayPreparationCandidate(ray,origin,angles);
        const auto& inputs=prepared.prepared;
        SourcePhysicsLeafStateCandidate state;state.ray=inputs.ray;state.search.direction={inputs.ray[0],inputs.ray[1],inputs.ray[2]};state.nearest=inputs.maximum;state.tolerance=inputs.tolerance;state.localDelta=inputs.localDelta;
        state.tree.midpoint=inputs.localMidpoint;state.tree.start=inputs.localStart;state.tree.direction=inputs.localDirection;state.tree.length=inputs.lengthMetres;state.tree.boxRadius=inputs.radius;
        SourcePhysicsSupportCacheCandidate cache;cache.generation=1;SourcePhysicsHullTraceStatisticsCandidate stats;auto treeState=state.tree;
        sourcePhysicsTreeTraversalCandidate(geometry.collisionTrees[0],treeState,[&](uint32_t,uint32_t ledge,SourcePhysicsTreeStateCandidate& working){
            state.tree=working;SourcePhysicsSupportContextCandidate shape;
            const uint32_t contents=contentsForGameData(geometry.solids[0].at(ledge).gameData);
            if(mask&contents){const auto* record=metadata.empty()?nullptr:&metadata.at(ledge);shape=sourcePhyPointSupportBindingCandidate(geometry.solids[0].at(ledge),ledge,record,inputs.rotation);if(shape.mode==0)++stats.walkSearches;else if(shape.mode==2)++stats.packedSearches;}
            const auto code=sourcePhysicsLeafCandidate(state,contents,mask,prepared.box,shape,cache);if(code<1||code>5)throw std::runtime_error("hull trace leaf disposition");++stats.outcomes[code-1];working=state.tree;
        });
        auto finite=[](float value){if(!std::isfinite(value))throw std::runtime_error("hull trace finalization overflow");return value;};
        auto result=state.trace;
        for(unsigned axis=0;axis<3;++axis){result.start[axis]=finite(ray.offset[axis]+ray.center[axis]);result.end[axis]=finite(finite(ray.delta[axis]*result.fraction)+result.start[axis]);}
        if(1.f>result.fraction||result.allSolid||result.startSolid)result.plane.distance=finite(finite(result.plane.normal[1]*result.end[1]+result.plane.normal[0]*result.end[0])+result.plane.normal[2]*result.end[2]);
        if(statistics)*statistics=stats;
        return result;
    }
};
