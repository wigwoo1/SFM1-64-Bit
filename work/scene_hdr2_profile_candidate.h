#pragma once
#include "scene_hdr2_lightmap_candidate.h"

enum class SceneHdr2FilterCandidate : uint32_t { Nearest, Linear };

// Immutable, explicit experimental configuration. It does not observe the
// original application, choose a native atlas, or alter saved session data.
class SceneHdr2ProfileCandidate {
    SourceHdr2ScenePolicyCandidate policy_;
    uint32_t padding_;
    uint8_t initialByte_;
    SceneHdr2FilterCandidate filter_;
    bool sessionExposure_;
    std::array<uint32_t,13> identity_{};
public:
    SceneHdr2ProfileCandidate(SourceHdr2ScenePolicyCandidate policy,uint32_t padding,
            uint8_t initialByte,SceneHdr2FilterCandidate filter,bool sessionExposure=false)
        :policy_(policy),padding_(padding),initialByte_(initialByte),filter_(filter),sessionExposure_(sessionExposure){
        validateSourceHdr2ScenePolicyCandidate(policy_);
        if(padding_>16383||(filter_!=SceneHdr2FilterCandidate::Nearest&&filter_!=SceneHdr2FilterCandidate::Linear))
            throw std::runtime_error("HDR2 explicit page/filter profile refused");
        auto bits=[](float value){uint32_t word;std::memcpy(&word,&value,4);return word;};
        identity_={uint32_t(policy_.selectedHdrType),policy_.encoding,bits(policy_.styleScale),
            bits(policy_.instance[0]),bits(policy_.instance[1]),bits(policy_.instance[2]),bits(policy_.instance[3]),
            policy_.configurationByte49,bits(policy_.exposure),padding_,initialByte_,uint32_t(filter_),uint32_t(sessionExposure_)};
    }
    SceneHdr2ProfileCandidate& operator=(const SceneHdr2ProfileCandidate&)=delete;
    const SourceHdr2ScenePolicyCandidate& policy()const noexcept{return policy_;}
    const std::array<uint32_t,13>& identity()const noexcept{return identity_;}
    bool usesSessionExposure()const noexcept{return sessionExposure_;}
    SourceBspLightmapSelectionCandidate dataset()const noexcept{return SourceBspLightmapSelectionCandidate::Hdr;}
    SceneHdr2FilterCandidate filter()const noexcept{return filter_;}
    SceneHdr2PagePlacementCandidate page(uint32_t width,uint32_t height)const{
        // Declared per-face pages with caller-selected padding bytes. In
        // particular, zero padding is not native edge extrusion or atlas parity.
        const uint64_t w=uint64_t(width)+uint64_t(padding_)*2,h=uint64_t(height)+uint64_t(padding_)*2;
        if(!width||!height||w>32768||h>32768)throw std::runtime_error("HDR2 profile page extent refused");
        return {uint32_t(w),uint32_t(h),padding_,padding_,initialByte_};
    }
};

// Pair policy and plan in one immutable owner, including empty/blank frames.
// This owner is CPU data only. Future GPU callers must retain their resources
// through every draw fence, even after an image's initial upload has completed.
class SceneHdr2PreparedFrameCandidate {
    std::shared_ptr<const SceneHdr2ProfileCandidate> profile_;
    std::shared_ptr<const SceneHdr2PlanCandidate> plan_;
    SceneHdr2PreparedFrameCandidate(std::shared_ptr<const SceneHdr2ProfileCandidate> profile,
            std::shared_ptr<const SceneHdr2PlanCandidate> plan)
        :profile_(std::move(profile)),plan_(std::move(plan)){}
public:
    SceneHdr2PreparedFrameCandidate& operator=(const SceneHdr2PreparedFrameCandidate&)=delete;
    const std::shared_ptr<const SceneHdr2ProfileCandidate>& profile()const noexcept{return profile_;}
    const std::shared_ptr<const SceneHdr2PlanCandidate>& plan()const noexcept{return plan_;}
    template<class Prepared>static std::shared_ptr<const SceneHdr2PreparedFrameCandidate> prepare(
            const Prepared& scene,std::shared_ptr<const SceneHdr2ProfileCandidate> profile,
            const std::function<void(const char*)>& hook={}){
        if(!profile)throw std::runtime_error("HDR2 frame profile required");
        auto policy=profile->policy();
        if(profile->usesSessionExposure()){
            if(scene.sessionExposure)policy.exposure=scene.sessionExposure->product;
            else if(scene.shot!=UINT32_MAX||!scene.upload.vertices.empty())
                throw std::runtime_error("session exposure snapshot missing from evaluated frame");
            // A prototype blank/gap frame has no camera and only clears.
            else policy.exposure=1;
        }
        auto plan=prepareSceneHdr2LightmapPlanCandidate(scene,policy,
            [&profile](const auto&,uint32_t,uint32_t w,uint32_t h){return profile->page(w,h);},hook);
        return std::shared_ptr<const SceneHdr2PreparedFrameCandidate>(
            new SceneHdr2PreparedFrameCandidate(std::move(profile),std::move(plan)));
    }
};
