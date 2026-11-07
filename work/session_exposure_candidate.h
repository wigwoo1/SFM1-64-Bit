#pragma once
#include "clip_hierarchy.h"
#include <cmath>

// Snapshot from an already evaluated graph. This is the observed IFM input
// product, not automatic exposure, tone mapping, bloom or a full post pipeline.
struct SessionExposureCandidate {
    uint32_t camera=UINT32_MAX,renderSettings=UINT32_MAX;
    float sessionScale=1,cameraScale=1,product=1;
    bool sessionAttribute=false,cameraAttribute=false;
};

inline SessionExposureCandidate readSessionExposureCandidate(
        const Graph& graph,uint32_t root,uint32_t camera){
    if(root>=graph.elements.size()||graph.strings.at(graph.elements[root].type)!="DmeFilmClip"||
       camera>=graph.elements.size()||graph.strings.at(graph.elements[camera].type)!="DmeCamera")
        throw std::runtime_error("session exposure requires the selected film and camera");
    SessionExposureCandidate result;result.camera=camera;
    const Element* owner=nullptr;
    for(const auto& element:graph.elements){
        auto active=hierarchyRefs(graph,element,"activeClip",1);
        if(active.size()==1&&active.front()==root){
            if(owner)throw std::runtime_error("ambiguous session exposure owner");
            owner=&element;
        }
    }
    if(owner){
        auto settings=hierarchyRefs(graph,*owner,"settings",1);
        if(!settings.empty()){
            auto render=hierarchyRefs(graph,graph.elements.at(settings.front()),"renderSettings",1);
            if(!render.empty())result.renderSettings=render.front();
        }
    }
    auto scale=[&](uint32_t id,bool& present){
        if(id==UINT32_MAX)return 1.f;
        const auto* attribute=namedAttribute(graph,graph.elements.at(id),"toneMapScale");
        if(!attribute)return 1.f;
        present=true;
        if(attribute->type!=3||attribute->values.size()!=1||attribute->values.front().wire.size()!=4)
            throw std::runtime_error("session exposure toneMapScale scalar schema");
        const float value=wireFloat(attribute->values.front());
        // Zero/negative native force-scale values can select other engine policy.
        // That policy is not reconstructed: refuse, never silently clamp it.
        if(!std::isfinite(value)||value<=0)
            throw std::runtime_error("session exposure supports positive finite toneMapScale only");
        return value;
    };
    result.sessionScale=scale(result.renderSettings,result.sessionAttribute);
    result.cameraScale=scale(camera,result.cameraAttribute);
    // The binary32 operands' exact product fits in binary64. Round once to
    // binary32, matching the observed x87 multiply followed by float storage.
    result.product=float(double(result.sessionScale)*double(result.cameraScale));
    if(!std::isfinite(result.product)||result.product<=0)
        throw std::runtime_error("session exposure product outside supported positive float range");
    return result;
}
