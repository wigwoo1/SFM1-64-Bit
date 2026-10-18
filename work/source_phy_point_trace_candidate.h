#pragma once
#include "source_phy_point_support_binding_candidate.h"
#include "source_physics_ray_preparation_candidate.h"
#include "source_physics_leaf_candidate.h"
#include "source_phy_aabb_candidate.h"
struct SourcePhysicsPointTraceStatisticsCandidate {
    std::vector<uint32_t>leafOrdinals;
    std::array<uint32_t,5>outcomes{};
    uint32_t packedSearches=0,walkSearches=0;
};
// Owned file-to-result point trace for the observed one-solid PHY format and
// default overload uses original contents provider1; callback overload accepts
// owned model-specific contents. No original DLL, recorded states or
// native pointers. Hull sweeps/game contents callbacks are separate scopes.
class SourcePhyPointTraceCandidate {
    SourcePhyCompactGeometryCandidate geometry;
    std::vector<SourcePhysicsSupportMetadataRecordCandidate>metadata;
public:
    explicit SourcePhyPointTraceCandidate(const std::vector<uint8_t>&bytes):geometry(bytes,true){
        if(geometry.solids.size()!=1||geometry.collisionTrees.size()!=1)throw std::runtime_error("point trace currently requires one PHY solid");sourcePhysicsValidateTreeCandidate(geometry.collisionTrees[0]);
        std::vector<bool>seen(geometry.solids[0].size(),false);for(const auto&n:geometry.collisionTrees[0])if(!n.raw[0]){if(n.ledge>=seen.size()||seen[n.ledge])throw std::runtime_error("point trace ledge mapping");seen[n.ledge]=true;}for(bool present:seen)if(!present)throw std::runtime_error("point trace unreachable ledge");metadata=sourcePhysicsSupportMetadataTableCandidate(geometry.solids[0]);
    }
    std::array<float,6>bounds(const std::array<float,3>&origin,const std::array<float,3>&angles)const{
        return sourcePhyAabbCandidate(geometry,metadata,origin,angles);
    }
    SourcePhysicsSearchTraceCandidate tracePoint(const std::array<float,3>&start,
        const std::array<float,3>&delta,const std::array<float,3>&origin,
        const std::array<float,3>&angles,uint32_t mask,
        SourcePhysicsSupportCacheCandidate*persistentCache=nullptr,
        SourcePhysicsPointTraceStatisticsCandidate*statistics=nullptr)const{
        return tracePointWithContents(start,delta,origin,angles,mask,[](uint32_t){return 1u;},persistentCache,statistics);
    }
    // Contents callback runs once per visited leaf, before mask filtering or
    // support preparation, as in original2A340. Callback side effects belong
    // to the caller; geometry/scratch/statistics remain transactional.
    template<class Contents>
    SourcePhysicsSearchTraceCandidate tracePointWithContents(const std::array<float,3>&start,
        const std::array<float,3>&delta,const std::array<float,3>&origin,
        const std::array<float,3>&angles,uint32_t mask,Contents&&contentsForGameData,
        SourcePhysicsSupportCacheCandidate*persistentCache=nullptr,
        SourcePhysicsPointTraceStatisticsCandidate*statistics=nullptr)const{
        const auto prepared=sourcePhysicsRayPreparationCandidate(start,delta,origin,angles);SourcePhysicsLeafStateCandidate state;state.ray=prepared.ray;state.search.direction={prepared.ray[0],prepared.ray[1],prepared.ray[2]};state.nearest=prepared.maximum;state.tolerance=prepared.tolerance;state.localDelta=prepared.localDelta;
        state.tree.midpoint=prepared.localMidpoint;state.tree.start=prepared.localStart;state.tree.direction=prepared.localDirection;state.tree.length=prepared.lengthMetres;state.tree.boxRadius=prepared.radius;
        SourcePhysicsSupportCacheCandidate cache;if(persistentCache)cache=*persistentCache;else cache.generation=1;SourcePhysicsPointTraceStatisticsCandidate stats;SourcePhysicsBoxSupportCandidate box;box.point=true;auto treeState=state.tree;
        sourcePhysicsTreeTraversalCandidate(geometry.collisionTrees[0],treeState,[&](uint32_t,uint32_t ledge,SourcePhysicsTreeStateCandidate&working){
            state.tree=working;stats.leafOrdinals.push_back(ledge);SourcePhysicsSupportContextCandidate shape;const uint32_t contents=contentsForGameData(geometry.solids[0].at(ledge).gameData);
            if(mask&contents){const auto*m=metadata.empty()?nullptr:&metadata.at(ledge);shape=sourcePhyPointSupportBindingCandidate(geometry.solids[0].at(ledge),ledge,m,prepared.rotation);if(shape.mode==0)++stats.walkSearches;else if(shape.mode==2)++stats.packedSearches;}
            const auto code=sourcePhysicsLeafCandidate(state,contents,mask,box,shape,cache);if(code<1||code>5)throw std::runtime_error("point trace leaf disposition");++stats.outcomes[code-1];working=state.tree;
        });
        auto finite=[](float x){if(!std::isfinite(x))throw std::runtime_error("point trace finalization overflow");return x;};
        auto result=state.trace;for(unsigned j=0;j<3;++j){result.start[j]=finite(start[j]+0.f);result.end[j]=finite(finite(delta[j]*result.fraction)+result.start[j]);}
        if(1.f>result.fraction||result.allSolid||result.startSolid)result.plane.distance=finite(finite(result.plane.normal[1]*result.end[1]+result.plane.normal[0]*result.end[0])+result.plane.normal[2]*result.end[2]);
        if(persistentCache)*persistentCache=cache;if(statistics)*statistics=std::move(stats);return result;
    }
};
