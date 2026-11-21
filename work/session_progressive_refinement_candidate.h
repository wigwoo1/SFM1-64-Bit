#pragma once

#include "session_exposure_candidate.h"
#include "source_refinement_policy_candidate.h"

inline SourceRefinementPlanCandidate readSessionProgressiveRefinementCandidate(
        const Graph& graph,uint32_t root,uint32_t camera,uint32_t packedRate) {
    const auto render=sessionRenderSettingsElementCandidate(graph,root);
    const auto refinement=render==UINT32_MAX?UINT32_MAX:
        hierarchySingleReference(graph,graph.elements.at(render),"ProgressiveRefinement");
    const auto scalar=[&](uint32_t element,const char* name,uint8_t type,size_t bytes)->const Value* {
        if(element==UINT32_MAX)return nullptr;
        const auto* a=namedAttribute(graph,graph.elements.at(element),name);
        if(!a)return nullptr;
        if(a->type!=type||a->values.size()!=1||a->values.front().wire.size()!=bytes)
            throw std::runtime_error(std::string("progressive refinement field schema: ")+name);
        return &a->values.front();
    };
    const auto flag=[&](const char* name){
        const auto* value=scalar(refinement,name,4,1);
        return value&&value->wire.front()!=0;
    };
    const auto integer=[&](uint32_t element,const char* name,int32_t fallback,uint8_t type=2){
        const auto* value=scalar(element,name,type,4);
        return value?wireInt(*value):fallback;
    };
    SourceRefinementSettingsCandidate settings;
    settings.enabled=flag("on");
    if(settings.enabled){
        settings.motionBlur=flag("useMotionBlur");
        settings.depthOfField=flag("useDepthOfField");
        settings.antialiasing=flag("useAntialiasing");
        if(settings.motionBlur&&flag("overrideMotionBlurQuality"))
            settings.motionQualityOverride=integer(refinement,"overrideMotionBlurQualityValue",1);
        if(settings.depthOfField&&flag("overrideDepthOfFieldQuality"))
            settings.depthQualityOverride=integer(refinement,"overrideDepthOfFieldQualityValue",1);
    }
    settings.overrideShutter=flag("overrideShutterSpeed");
    if(settings.overrideShutter){
        const auto* value=namedAttribute(graph,graph.elements.at(refinement),"overrideShutterSpeedValue");
        if(value){
            if(value->type==7||value->type==2)settings.shutterOverrideTicks=integer(refinement,"overrideShutterSpeedValue",0,value->type);
            else if(value->type==4)settings.shutterOverrideTicks=flag("overrideShutterSpeedValue")?10000:0;
            else settings.shutterOverrideTicks=sourceShutterSecondsToTicksCandidate(wireFloat(*scalar(refinement,"overrideShutterSpeedValue",3,4)));
        }
    }
    std::optional<SourceRefinementCameraCandidate> optics;
    if(camera!=UINT32_MAX){
        if(graph.strings.at(graph.elements.at(camera).type)!="DmeCamera")
            throw std::runtime_error("progressive refinement requires a camera");
        optics=SourceRefinementCameraCandidate{
            settings.motionBlur&&!settings.motionQualityOverride?integer(camera,"motionBlurQuality",0):0,
            settings.depthOfField&&!settings.depthQualityOverride?integer(camera,"depthOfFieldQuality",0):0,
            settings.overrideShutter?0:integer(camera,"shutterSpeed",208,7)};
    }
    return sourceRefinementPlanCandidate(settings,optics,packedRate);
}
