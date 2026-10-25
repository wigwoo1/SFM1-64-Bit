#pragma once
#include "ifm_draw_frames_candidate.h"
#include "ifm_bone_world_palette_candidate.h"
// Original1023CE60: public time is unused; every call prepares with INT_MIN.
// Services own storage and native profiling/clock/prepare dependencies.
template<class Services>
bool ifmSetupBonesWrapperCandidate(int32_t maximum,uint32_t mask,uint32_t publicTimeBits,
    std::vector<RecoveredMatrix3x4>*output,Services&services){
    (void)publicTimeBits;services.begin();
    try{
        if(services.hasClock())(void)services.clockTick();
        bool result=services.prepare(maximum,mask,INT32_MIN);
        if(result&&output){const auto&cache=services.cachedPalette();
            if(maximum<0||uint64_t(maximum)<cache.size()){services.warnCapacity(maximum,uint32_t(cache.size()));result=false;}
            else {if(output->size()<cache.size())throw std::runtime_error("bone wrapper output span");std::copy(cache.begin(),cache.end(),output->begin());}
        }services.end();return result;
    }catch(...){services.end();throw;}
}
// Root calculation used by10241820 after1023E0B0, with separately supplied
// prepared nonprocedural poses/precomputed palette. Bone-flex drivers require
// another provider and must not be silently omitted by a caller.
inline RecoveredMatrix3x4 ifmSetupBoneRootCandidate(const RecoveredMatrix3x4&world,float localScale){
    const auto frame=ifmDecomposeFrameCandidate(world);auto root=ifmBoundsAngleMatrixCandidate(frame.angles,frame.position);
    if(!std::isfinite(localScale))throw std::runtime_error("bone setup root scale");
    if(localScale!=1)for(auto&r:root)for(unsigned c=0;c<3;++c)r[c]*=localScale;checkIfmMatrixFiniteCandidate(root);return root;
}
inline void ifmSetupFlexPrefixCandidate(const std::vector<float>&input,std::vector<float>&cache){
    if(input.size()>256||cache.size()>256)throw std::runtime_error("bone setup flex prefix limit");
    if(cache.size()<input.size())cache.resize(input.size(),0);std::copy(input.begin(),input.end(),cache.begin());
}
// Loaded, stable prepared-pose contract of10241820. Providers bind headers,
// typed model/transform inputs, already verified optional precomputed matrices,
// clock and storage. Bone-flex drivers/procedural poses require explicit support.
template<class Providers>
bool ifmPrepareSetupBonesCandidate(Providers&p){
    p.lock();try{
        if(!p.hasHeader()&&p.canLoadHeader())p.loadHeader();
        if(!p.hasHeader()){p.unlock();return false;}
        p.updateRootFrame();
        if(!p.hasTypedModel()){p.unlock();return true;}
        p.synchronize();if(!p.localTransformCount()){p.unlock();return true;}
        p.validateSupportedPose();p.copyFlexInputs();
        const auto poses=p.localPoses();const auto root=ifmSetupBoneRootCandidate(p.modelWorld(),p.rootScale());
        if(p.hasClock()){(void)p.clockTick();p.clockContext();}
        const auto palette=ifmBoneWorldPaletteCandidate(poses,root,0x7ff00,p.precomputedPalette());
        p.storePalette(palette);p.unlock();return true;
    }catch(...){p.unlock();throw;}
}
