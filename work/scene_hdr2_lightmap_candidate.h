#pragma once
#include "scene_base_lightmap_candidate.h"
#include "source_bsp_accumulated_lightmap_candidate.h"
#include "source_plain_lightmap_sampling_candidate.h"
#include "source_hdr_lightmap_scale_candidate.h"
#include "source_lightmap_instance_modulation_candidate.h"
#include "native_lightmap_hdr2_region_candidate.h"
#include <functional>

// Explicit controlled profile. Encoding IDs describe CPU byte storage, not an
// observed D3D device format or an inferred Vulkan/native HDR mode.
struct SourceHdr2ScenePolicyCandidate {
    int32_t selectedHdrType;
    uint32_t encoding;
    float styleScale;
    std::array<float,4> instance;
    uint8_t configurationByte49;
    float exposure;
};
struct SceneHdr2PagePlacementCandidate {
    uint32_t width,height,x,y;
    uint8_t initialByte;
};
using SceneHdr2PagePlacementFunctionCandidate=std::function<SceneHdr2PagePlacementCandidate(
    const SourceBspLightmapSnapshotCandidate&,uint32_t,uint32_t,uint32_t)>;
struct SceneHdr2FaceCandidate {
    std::shared_ptr<const SourceBspAccumulatedLightmapCandidate> accumulated;
    SceneHdr2PagePlacementCandidate page;
    std::vector<uint8_t> bytes;
};
struct SceneHdr2BatchCandidate {
    SceneGpuDrawCandidate draw;
    size_t faceResource=SIZE_MAX;
    std::string unadmittedReason;
};
struct SceneHdr2PlanCandidate {
    SourceHdr2ScenePolicyCandidate policy;
    float hdrScale=0;
    SourceInstanceModulationPlanCandidate modulation;
    std::vector<SceneHdr2FaceCandidate> faces;
    std::vector<std::array<float,2>> lightUvs;
    std::vector<SceneHdr2BatchCandidate> opaque,translucent;
};

inline void validateSourceHdr2ScenePolicyCandidate(const SourceHdr2ScenePolicyCandidate& policy){
    const uint32_t fpControl=_mm_getcsr();
    if((fpControl&0xe040u)!=0||(fpControl&0x1f80u)!=0x1f80u)throw std::runtime_error("HDR2 floating-point control refused");
    if(policy.selectedHdrType!=2||(policy.encoding!=36&&policy.encoding!=113)||
       !std::isfinite(policy.styleScale)||policy.styleScale<0||policy.styleScale>16||
       !std::isfinite(policy.exposure)||policy.exposure<=0||policy.instance[3]!=1)
        throw std::runtime_error("HDR2 explicit opaque profile required");
    for(float v:policy.instance)if(!std::isfinite(v)||v<0||v>16)throw std::runtime_error("HDR2 instance range");
}

// Return a complete immutable owner. Placement is requested once per unique
// snapshot/face in this plan. No cross-plan cache or native atlas equivalence.
template<class Prepared>std::shared_ptr<const SceneHdr2PlanCandidate> prepareSceneHdr2LightmapPlanCandidate(
    const Prepared& scene,const SourceHdr2ScenePolicyCandidate& policy,
    const SceneHdr2PagePlacementFunctionCandidate& placement,
    const std::function<void(const char*)>& hook={}){
    auto stage=[&](const char* s){if(hook)hook(s);};
    validateSourceHdr2ScenePolicyCandidate(policy);
    if(!placement)throw std::runtime_error("HDR2 explicit placement required");
    const auto& u=scene.upload;validateSceneShadingCandidate(u,true);validateSceneBspLightmapsCandidate(u);
    if(scene.materialAlpha.size()!=u.materials.size())throw std::runtime_error("HDR2 material alpha coverage");
    SceneHdr2PlanCandidate next{policy};
    if(!sourceHdrLightmapScaleCandidate(next.hdrScale,policy.selectedHdrType))throw std::runtime_error("HDR2 scale refused");
    const auto commands=sourceLightmapInstanceCommandsCandidate(next.hdrScale);
    if(!sourceInstanceModulationPlanCandidate(next.modulation,commands.data(),commands.size(),policy.instance,policy.configurationByte49,22,48))throw std::runtime_error("HDR2 modulation refused");
    next.lightUvs.resize(u.vertices.size());std::vector<size_t> uvOwners(u.vertices.size(),SIZE_MAX);
    std::map<std::pair<const SourceBspLightmapSnapshotCandidate*,uint32_t>,size_t> cached;
    auto append=[&](const auto& list,auto& output,bool translucent){for(const auto& draw:list){
        if(!draw.indexCount||draw.indexCount%3||draw.firstIndex>u.indices.size()||draw.indexCount>u.indices.size()-draw.firstIndex||draw.materialSlot>=u.materials.size())throw std::runtime_error("HDR2 draw span");
        const auto& address=u.materials.at(draw.materialSlot);
        if(address.model!=draw.model||address.material!=draw.materialIndex)throw std::runtime_error("HDR2 draw material identity");
        const auto& inputs=scene.materialInputs.at(draw.model);const auto& material=inputs.materials.at(draw.materialIndex);
        const SceneBspLightmapAssociationCandidate* association=nullptr;
        for(const auto& a:u.bspLightmaps)if(a.model==draw.model&&draw.firstIndex>=a.firstIndex&&uint64_t(draw.firstIndex)+draw.indexCount<=uint64_t(a.firstIndex)+a.indexCount){if(association)throw std::runtime_error("HDR2 ambiguous association");association=&a;}
        if(association){const auto& range=inputs.ranges.at(draw.range);if(range.materialIndex!=draw.materialIndex||range.indexCount!=draw.indexCount||uint64_t(association->firstIndex)+range.firstIndex!=draw.firstIndex||inputs.indices!=association->snapshot->localIndices)throw std::runtime_error("HDR2 retained range identity");}
        std::string reason;
        if(!sceneBaseLightmapMaterialCandidate(material))reason="material shader outside plain HDR2 subset";
        else if(!association)reason="no retained BSP association";
        else if(translucent)reason="translucent range";
        else{try{validateSceneBaseLightmapMaterialCandidate(material,scene.materialAlpha.at(draw.materialSlot));}catch(const std::runtime_error& e){reason=e.what();}}
        if(!reason.empty()){output.push_back({draw,SIZE_MAX,reason});continue;}
        const auto& a=*association;const auto& s=*a.snapshot;const auto& range=inputs.ranges.at(draw.range);
        if(range.eyeball!=-1){output.push_back({draw,SIZE_MAX,"eyeball range"});continue;}
        if(s.state!=SourceBspLightmapSnapshotStateCandidate::Available){output.push_back({draw,SIZE_MAX,s.unavailableReason});continue;}
        const auto& data=*s.lightmaps;
        if(!data.pairedFaceLightingTables()||data.faceLump()!=58||data.lightingLump()!=53){output.push_back({draw,SIZE_MAX,"requires paired HDR faces58/lighting53"});continue;}
        for(uint32_t j=0;j<draw.indexCount;j+=3){
            const auto face=s.orderedTriangleFaces.at((draw.firstIndex-a.firstIndex+j)/3);
            const auto slot=size_t(face-data.firstFace());const auto& f=data.face(slot);size_t resource=SIZE_MAX;std::string why;
            for(size_t k=0;k<3;++k){const auto global=u.indices.at(size_t(draw.firstIndex)+j+k);const auto local=global-a.firstVertex;const auto& v=u.vertexShading.at(global);
                if(global<a.firstVertex||local>=a.vertexCount||v.origin!=SceneShadingOriginCandidate::WorldGeometry||v.model!=a.model||v.sourceVertex!=local||s.vertexLuxels.at(local).faceIndex!=face)throw std::runtime_error("HDR2 vertex provenance");
                // Only non-displaced world output equals the original source floats.
                if(f.displacementIndex==-1&&data.luxel(slot,u.vertices.at(global).position)!=s.vertexLuxels.at(local).luxel)throw std::runtime_error("HDR2 source position/luxel identity");
            }
            if(f.availability!=SourceBspLightmapAvailabilityCandidate::Samples||f.styleCount!=1||f.styles[0]!=0||f.planesPerStyle!=1||f.flags!=0||f.displacementIndex!=-1)why="face outside static style0 plain non-displaced subset";
            else if(f.samplesPerPlane>16384||uint64_t(f.size[0]+1)*8>65535)why="face outside accumulated plane limits";
            if(why.empty()){
                const auto key=std::make_pair(&s,face);auto found=cached.find(key);
                if(found==cached.end()){
                    auto plane=SourceBspAccumulatedLightmapCandidate::prepare(a.snapshot,face,material.source.syntax,policy.styleScale,SourceLightmapAccumulationUnitsCandidate::TexLightLinear);stage("face-accumulated");
                    const auto page=placement(s,face,plane->width(),plane->height());
                    if(!page.width||!page.height||page.width>32768||page.height>32768||uint64_t(page.x)+plane->width()>page.width||uint64_t(page.y)+plane->height()>page.height||page.x>32767||page.y>32767)throw std::runtime_error("HDR2 explicit page placement");
                    size_t rowBytes,totalBytes;if(!native_lightmap_hdr2_region_detail::multiply(page.width,8,rowBytes)||!native_lightmap_hdr2_region_detail::multiply(rowBytes,page.height,totalBytes))throw std::runtime_error("HDR2 page byte overflow");
                    std::vector<NativeLightmapHdr2BitsCandidate> bits(plane->pixels().size());for(size_t n=0;n<bits.size();++n)std::memcpy(bits[n].data(),plane->pixels()[n].data(),16);
                    SceneHdr2FaceCandidate prepared{plane,page,std::vector<uint8_t>(totalBytes,page.initialByte)};
                    if(!nativeLightmapHdr2RegionCandidate(policy.encoding,{&bits,nullptr,nullptr,nullptr},{plane->width(),plane->height(),page.x,page.y,rowBytes},prepared.bytes))throw std::runtime_error("HDR2 page encoding refused");stage("page-encoded");
                    resource=next.faces.size();next.faces.push_back(std::move(prepared));cached.emplace(key,resource);
                }else resource=found->second;
                const auto& page=next.faces.at(resource).page;
                for(size_t k=0;k<3;++k){const auto global=u.indices.at(size_t(draw.firstIndex)+j+k);SourcePlainLightmapSamplingInputCandidate input{};input.axes=f.axes;input.minima=f.minima;input.size=f.size;input.position=u.vertices.at(global).position;input.surfaceFlags=f.flags;input.pageOffset={int32_t(page.x),int32_t(page.y)};input.pageDimensions={page.width,page.height};
                    std::array<float,2> uv;if(!sourcePlainLightmapSamplingCandidate(uv,input))throw std::runtime_error("HDR2 UV control/range refused");if(uvOwners[global]!=SIZE_MAX&&(uvOwners[global]!=resource||next.lightUvs[global]!=uv))throw std::runtime_error("HDR2 conflicting vertex UV ownership");uvOwners[global]=resource;next.lightUvs[global]=uv;}
            }
            auto split=draw;split.firstIndex+=j;split.indexCount=3;
            if(!output.empty()){auto& prev=output.back();if(prev.faceResource==resource&&prev.unadmittedReason==why&&prev.draw.model==split.model&&prev.draw.range==split.range&&prev.draw.materialSlot==split.materialSlot&&prev.draw.materialIndex==split.materialIndex&&uint64_t(prev.draw.firstIndex)+prev.draw.indexCount==split.firstIndex){prev.draw.indexCount+=3;continue;}}
            output.push_back({split,resource,why});
        }
    }};append(u.opaque,next.opaque,false);append(u.translucent,next.translucent,true);stage("plan-ready");
    return std::make_shared<const SceneHdr2PlanCandidate>(std::move(next));
}
