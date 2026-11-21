#pragma once
#include "source_engine_point_trace_candidate.h"
#include "source_loose_collision_scene_assets_candidate.h"
#include "source_static_prop_point_spatial_candidate.h"
#include "source_world_ray_trace_candidate.h"

struct SourceParticleStaticPropTraceCandidate {
    SourcePhysicsSearchTraceCandidate hit;
    float worldFraction = 1.f;
    size_t overflow = 0;
};

// Prepared BSP static props in the existing checked loose-mount scope. The
// scene owner supplies the immutable world query from the same BSP.
class SourceParticleStaticPropSceneCandidate {
    SourceStaticPropPointSpatialCandidate props_;

    static std::vector<uint64_t> ids(size_t count){
        std::vector<uint64_t> result;
        result.reserve(count);
        for(size_t i=0;i<count;++i)result.push_back(i+1);
        return result;
    }
    static SourceStaticPropPointSpatialCandidate prepare(
        const std::vector<uint8_t>& bytes,const std::vector<std::filesystem::path>& mounts){
        SourceBspStaticPropsCandidate props(bytes);
        SourceLooseCollisionSceneAssetsCandidate assets(props,mounts,ids(props.models().size()));
        return SourceStaticPropPointSpatialCandidate(props,assets.models(),ids(props.records().size()));
    }

    struct Services {
        const SourcePreparedWorldRayCandidate& world;
        const SourceStaticPropPointSpatialCandidate& props;
        float worldFraction = 1.f;
        size_t overflow = 0;

        SourcePhysicsSearchTraceCandidate traceWorld(const SourceWorldLightRayCandidate& ray,uint32_t mask){
            const auto result=sourceWorldRayTraceWithSweptFlagCandidate(world,ray.start,ray.delta,mask,ray.swept);
            const auto& trace=result.state.trace;
            SourcePhysicsSearchTraceCandidate converted;
            converted.start=result.startPosition;
            converted.end=result.endPosition;
            converted.fraction=worldFraction=trace.fraction;
            converted.fractionLeftSolid=trace.leftSolid;
            converted.allSolid=trace.allSolid;
            converted.startSolid=trace.startSolid;
            converted.contents=trace.contents;
            converted.plane.normal={trace.plane[0],trace.plane[1],trace.plane[2]};
            converted.plane.distance=trace.plane[3];
            converted.planeTags=trace.planeTags;
            converted.displacementFlags=result.state.displacementFlags;
            return converted;
        }
        auto enumerateCandidates(const SourceWorldLightRayCandidate& ray){return props.enumerateCandidates(ray);}
        void candidateOverflow(size_t excess){overflow=excess;}
        auto resolveCollideable(uintptr_t entity){return props.runtimeLease(entity);}
        uint32_t referenceHandle(uintptr_t entity){return props.runtimeLease(entity).binding().referenceHandle;}
        SourcePhysicsSearchTraceCandidate clipToCollideable(const SourceWorldLightRayCandidate& ray,
            uint32_t mask,const SourceStaticPropPointLeaseCandidate& lease){
            // Physics attributes prop hits to the world and stores the prop in hitBox.
            return lease.clip(ray,mask,0).trace;
        }
    };
public:
    SourceParticleStaticPropSceneCandidate(
        const std::vector<uint8_t>& bytes,const std::vector<std::filesystem::path>& mounts)
        :props_(prepare(bytes,mounts)){}
    size_t registeredCount()const{return props_.registeredCount();}

    SourceParticleStaticPropTraceCandidate trace(const SourcePreparedWorldRayCandidate& world,
        const SourceWorldLightRayCandidate& ray,uint32_t mask)const{
        Services services{world,props_};
        auto hit=sourceEngineParticlePointTraceCandidate(ray,mask,services,[](uintptr_t,uint32_t)->bool{
            throw std::logic_error("static prop trace received a non-static candidate");
        });
        return {hit,services.worldFraction,services.overflow};
    }
};
