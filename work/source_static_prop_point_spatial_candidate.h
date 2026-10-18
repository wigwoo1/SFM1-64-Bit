#pragma once
#include "source_static_prop_point_registry_candidate.h"
#include "source_spatial_point_ray_adapter_candidate.h"

struct SourceStaticPropPointRegistrationCandidate {
    uint64_t entityId=0;
    uint32_t diskSolidType=0,effectiveSolidType=0;
    uint16_t partitionHandle=0xffff;
    std::optional<std::array<float,6>>physicalBounds;
};
// Immutable initial registration for checked solid0/solid6, one-solid assets.
// Original101C4580 disables solid6 when completed collision loading is missing.
// Preserve that runtime decision separately from the unchanged disk binding.
// This composes ownership + GetAABB + point spatial order, NOT full scene tracing:
// material resolution, world merge, dynamic registration and solid2 stay gated.
class SourceStaticPropPointSpatialCandidate {
    SourceStaticPropPointRegistryCandidate registry_;
    std::vector<SourceStaticPropPointRegistrationCandidate>registrations_;
    std::unordered_map<uint64_t,size_t>indices_;
    SourceSpatialPointPartitionCandidate partition_;
    static std::vector<SourceStaticPropPointRegistrationCandidate>prepare(
        const SourceStaticPropPointRegistryCandidate&registry,const std::vector<uint64_t>&ids){
        std::vector<SourceStaticPropPointRegistrationCandidate>out;out.reserve(ids.size());
        size_t inserted=0;
        for(uint64_t id:ids){
            const auto lease=registry.retain(id);const auto&binding=lease.binding();
            SourceStaticPropPointRegistrationCandidate r;r.entityId=id;
            r.diskSolidType=r.effectiveSolidType=binding.solidType;
            if(binding.solidType==6){
                const auto&asset=*lease.assetOwner();const auto&shape=asset.sharedShape();
                if(!shape){
                    if(!asset.missingInConfiguredLooseMounts())throw std::runtime_error("static prop registration collision unresolved");
                    r.effectiveSolidType=0;
                }else{
                    if(inserted>=65535)throw std::runtime_error("static prop initial partition handle domain exhausted");
                    r.physicalBounds=shape->bounds(binding.origin,binding.angles);
                    r.partitionHandle=uint16_t(inserted++);
                }
            }else if(binding.solidType!=0)throw std::runtime_error("static prop spatial registration requires solid0/solid6");
            out.push_back(r);
        }
        return out;
    }
    static std::vector<SourceSpatialPointEntryCandidate>entries(
        const std::vector<SourceStaticPropPointRegistrationCandidate>&registrations){
        std::vector<SourceSpatialPointEntryCandidate>out;
        for(const auto&r:registrations)if(r.physicalBounds)out.push_back({r.entityId,0x65,*r.physicalBounds});
        return out;
    }
public:
    SourceStaticPropPointSpatialCandidate(const SourceBspStaticPropsCandidate&props,
        std::vector<SourceStaticPropPointModelCandidate>models,const std::vector<uint64_t>&entityIds)
        :registry_(props,std::move(models),entityIds),registrations_(prepare(registry_,entityIds)),
        partition_(entries(registrations_)){
        for(size_t i=0;i<registrations_.size();++i)indices_.emplace(registrations_[i].entityId,i);
    }
    size_t size()const{return registrations_.size();}
    size_t registeredCount()const{return partition_.size();}
    const auto&registrationAt(size_t ordinal)const{return registrations_.at(ordinal);}
    const SourceStaticPropPointRegistrationCandidate&registration(uint64_t id)const{
        const auto found=indices_.find(id);if(found==indices_.end())throw std::runtime_error("static prop spatial identity not in snapshot");
        return registrations_[found->second];
    }
    // Explicitly named: callers must not confuse original disk solid6 with a
    // prepared runtime disabled solid0. Collision services use runtimeLease.
    SourceStaticPropPointLeaseCandidate diskLease(uint64_t id)const{return registry_.retain(id);}
    SourceStaticPropPointLeaseCandidate runtimeLease(uint64_t id)const{
        auto lease=registry_.retain(id);lease.binding_.solidType=registration(id).effectiveSolidType;return lease;
    }
    SourceStaticPropCollisionBindingCandidate effectiveBinding(uint64_t id)const{
        return runtimeLease(id).binding();
    }
    const auto&partition()const{return partition_;}
    std::vector<uint64_t>enumerateCandidates(const SourceWorldLightRayCandidate&ray,
        uint32_t mask=0x65,size_t stopAfter=0)const{
        return sourceSpatialPointEnumerateRayCandidate(partition_,ray,mask,stopAfter);
    }
};
