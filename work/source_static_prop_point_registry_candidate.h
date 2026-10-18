#pragma once
#include "source_bsp_static_props_candidate.h"
#include "source_studio_loose_collision_asset_candidate.h"
#include "source_engine_studio_point_clip_candidate.h"
#include <optional>
#include <unordered_map>
#include <utility>

struct SourceStaticPropPointModelCandidate {
    std::string name;
    uint64_t modelId=0;
    std::shared_ptr<const SourceStudioLooseCollisionAssetCandidate>asset;
    // A resolved runtime property, NOT studiohdr's on-disk cache word.
    std::optional<uint16_t>surfaceProperty;
};
class SourceStaticPropPointRegistryCandidate;
// A value/ownership snapshot. It survives registry/source-buffer destruction;
// no address into a movable vector or original 32bit object is retained.
class SourceStaticPropPointLeaseCandidate {
    friend class SourceStaticPropPointRegistryCandidate;
    friend class SourceStaticPropPointSpatialCandidate;
    SourceStaticPropCollisionBindingCandidate binding_;
    std::shared_ptr<const SourceStudioLooseCollisionAssetCandidate>asset_;
    std::optional<uint16_t>surface_;
    SourceStaticPropPointLeaseCandidate(SourceStaticPropCollisionBindingCandidate b,
        std::shared_ptr<const SourceStudioLooseCollisionAssetCandidate>a,std::optional<uint16_t>s)
        :binding_(b),asset_(std::move(a)),surface_(s){}
public:
    const auto&binding()const{return binding_;}
    const auto&assetOwner()const{return asset_;}
    SourceEngineStudioPointClipResultCandidate clip(const SourceWorldLightRayCandidate&ray,
        uint32_t mask,uint64_t worldEntityId,SourcePhysicsSupportCacheCandidate*scratch=nullptr)const{
        if(mask&0x40000000u)throw std::runtime_error("static prop hitbox clipping unsupported");
        if(binding_.solidType==0){
            // Original1016F300 initialized point trace, no collision dispatch.
            SourceEngineStudioPointClipResultCandidate answer;
            for(unsigned j=0;j<3;++j){if(!std::isfinite(ray.start[j])||!std::isfinite(ray.delta[j]))throw std::runtime_error("nonfinite static prop ray");answer.trace.start[j]=ray.start[j]+0.f;answer.trace.end[j]=answer.trace.start[j]+ray.delta[j];if(!std::isfinite(answer.trace.end[j]))throw std::runtime_error("static prop ray overflow");}
            return answer;
        }
        if(binding_.solidType!=6)throw std::runtime_error("static prop solid type not implemented for point clipping");
        if(!surface_)throw std::runtime_error("static prop surface property unresolved");
        const auto&shape=asset_->sharedShape();if(!shape&&!asset_->missingInConfiguredLooseMounts())throw std::runtime_error("static prop collision availability unresolved");
        SourceEngineStudioPointClipParametersCandidate p;p.origin=binding_.origin;p.angles=binding_.angles;p.mask=mask;p.solidFlags=binding_.solidFlags;p.referenceHandle=binding_.referenceHandle;p.surfaceProperty=*surface_;p.worldEntityId=worldEntityId;
        return sourceEngineStudioPointClipCandidate(shape.get(),asset_->studio(),ray,p,scratch);
    }
};
// Immutable prepared registry for the current explicit loose-asset scope.
// IDs are supplied by the scene owner and must include any needed generation;
// they are not pointers or encoded 32bit Source handles. Disk ordinal defines
// a handle/index only: this class deliberately supplies NO enumeration order.
// Map archives, material-property resolution and a native spatial partition
// must be connected before this registry can drive general scene tracing.
class SourceStaticPropPointRegistryCandidate {
    struct Entry {uint64_t entityId=0;uint16_t model=0;SourceStaticPropCollisionBindingCandidate binding;};
    std::vector<SourceStaticPropPointModelCandidate>models_;
    std::vector<Entry>entries_;
    std::unordered_map<uint64_t,size_t>indices_;
public:
    SourceStaticPropPointRegistryCandidate(const SourceBspStaticPropsCandidate&props,
        std::vector<SourceStaticPropPointModelCandidate>models,const std::vector<uint64_t>&entityIds)
        :models_(std::move(models)){
        if(models_.size()!=props.models().size()||entityIds.size()!=props.records().size())throw std::runtime_error("static prop registry count mismatch");
        std::unordered_map<uint64_t,size_t>modelIdentities;
        for(size_t i=0;i<models_.size();++i){const auto&m=models_[i];if(m.name!=props.models()[i]||!m.modelId||!m.asset)throw std::runtime_error("static prop model binding incomplete");auto [found,inserted]=modelIdentities.emplace(m.modelId,i);if(!inserted){const auto&other=models_[found->second];if(other.asset!=m.asset||other.surfaceProperty!=m.surfaceProperty)throw std::runtime_error("inconsistent shared static prop model identity");}}
        entries_.reserve(entityIds.size());indices_.reserve(entityIds.size());
        for(size_t i=0;i<entityIds.size();++i){const auto&r=props.records()[i];if(!entityIds[i]||!indices_.emplace(entityIds[i],i).second)throw std::runtime_error("static prop entity identity duplicate/zero");entries_.push_back({entityIds[i],r.model,sourceStaticPropCollisionBindingCandidate(r,i,models_[r.model].modelId)});}
    }
    size_t size()const{return entries_.size();}
    size_t modelCount()const{return models_.size();}
    SourceStaticPropPointLeaseCandidate retain(uint64_t entityId)const{
        const auto found=indices_.find(entityId);if(found==indices_.end())throw std::runtime_error("static prop entity identity not in registry");const auto&e=entries_[found->second];const auto&m=models_[e.model];return {e.binding,m.asset,m.surfaceProperty};
    }
    uint32_t referenceHandle(uint64_t entityId)const{return retain(entityId).binding().referenceHandle;}
};
