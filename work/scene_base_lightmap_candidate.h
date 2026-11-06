#pragma once
#include "scene_gpu_upload_candidate.h"
#include "model_render_inputs_candidate.h"
#include "source_bsp_base_lightmap_candidate.h"
#include <map>

struct SceneBaseLightmapOptionsCandidate {
    std::array<float,3> modulation;
    explicit SceneBaseLightmapOptionsCandidate(std::array<float,3> value):modulation(value){}
};
inline void validateSceneBaseLightmapOptionsCandidate(const SceneBaseLightmapOptionsCandidate& options){
    for(float value:options.modulation)if(!std::isfinite(value)||value<0||value>16)throw std::runtime_error("explicit base lightmap modulation range");
}
inline bool sceneBaseLightmapMaterialCandidate(const ModelRenderMaterialCandidate& m){return vmtAsciiLowerCandidate(m.source.syntax.key)=="lightmappedgeneric";}
inline void validateSceneBaseLightmapInputsCandidate(const ModelRenderMaterialCandidate& m,float alpha){
    if(alpha!=1||m.translucent||m.alphaCutoff!=0||m.vertexTransition||m.selfIllum.enabled||m.irisImage.width||m.unlitTwoTexture||m.additive||m.source.baseTextureMissing||!m.colorSpace.baseSrgb||m.baseTextureTransform!=materialUvTransformCandidate(m.source.syntax))
        throw std::runtime_error("unsupported base lightmap prepared material feature");
    // vertexLighting is legacy-preview bookkeeping on retained world materials;
    // this path consumes raw lightmaps, never those preview colors.
    if(!m.preparedBase.cpu)throw std::runtime_error("base lightmap prepared VTF unavailable");
    const auto& image=*m.preparedBase.cpu->image();const auto& p=image.plan();
    if((image.format()!=0&&image.format()!=13&&image.format()!=15)||!p[9]||m.preparedBase.sourceMip>=p[9]||m.preparedBase.frame>=p[10])throw std::runtime_error("unsupported base lightmap VTF format/frame/mip");
}
inline void validateSceneBaseLightmapMaterialCandidate(const ModelRenderMaterialCandidate& m,float alpha){
    validateSourceBaseLightmapMaterialCandidate(m.source.syntax);
    validateSceneBaseLightmapInputsCandidate(m,alpha);
}
struct SceneBaseLightmapBatchCandidate {SceneGpuDrawCandidate draw;size_t plane=SIZE_MAX;};
struct SceneBaseLightmapPlanCandidate {
    std::vector<std::array<float,2>> luxels;
    std::vector<std::shared_ptr<const SourceBspBaseLightmapCandidate>> planes;
    std::vector<SceneBaseLightmapBatchCandidate> opaque,translucent;
};
template<class Prepared>SceneBaseLightmapPlanCandidate prepareSceneBaseLightmapPlanCandidate(const Prepared& scene,const SceneBaseLightmapOptionsCandidate& options){
    validateSceneBaseLightmapOptionsCandidate(options);const auto& u=scene.upload;
    validateSceneShadingCandidate(u,true);validateSceneBspLightmapsCandidate(u);
    if(scene.materialAlpha.size()!=u.materials.size())throw std::runtime_error("base lightmap material alpha coverage");
    SceneBaseLightmapPlanCandidate next;next.luxels.resize(u.vertices.size());
    std::map<std::pair<const SourceBspLightmapSnapshotCandidate*,uint32_t>,size_t> cached;
    auto append=[&](const auto& list,auto& output,bool translucent){for(const auto& draw:list){
        if(!draw.indexCount||draw.indexCount%3||draw.firstIndex>u.indices.size()||draw.indexCount>u.indices.size()-draw.firstIndex||draw.materialSlot>=u.materials.size())throw std::runtime_error("base lightmap draw span");
        const auto& address=u.materials.at(draw.materialSlot);const auto& inputs=scene.materialInputs.at(draw.model);const auto& range=inputs.ranges.at(draw.range);
        if(address.model!=draw.model||address.material!=draw.materialIndex||range.materialIndex!=draw.materialIndex||range.indexCount!=draw.indexCount)throw std::runtime_error("base lightmap draw/material identity");
        const auto& material=inputs.materials.at(draw.materialIndex);
        if(!sceneBaseLightmapMaterialCandidate(material)){output.push_back({draw,SIZE_MAX});continue;}
        validateSceneBaseLightmapMaterialCandidate(material,scene.materialAlpha.at(draw.materialSlot));
        if(translucent||range.eyeball!=-1)throw std::runtime_error("base lightmap requires opaque world range");
        const SceneBspLightmapAssociationCandidate* association=nullptr;
        for(const auto& a:u.bspLightmaps)if(a.model==draw.model&&draw.firstIndex>=a.firstIndex&&uint64_t(draw.firstIndex)+draw.indexCount<=uint64_t(a.firstIndex)+a.indexCount){if(association)throw std::runtime_error("ambiguous base lightmap association");association=&a;}
        if(!association)throw std::runtime_error("base lightmap draw lacks source-world association");
        const auto& a=*association;const auto& s=*a.snapshot;
        if(s.state!=SourceBspLightmapSnapshotStateCandidate::Available||inputs.indices!=s.localIndices||uint64_t(a.firstIndex)+range.firstIndex!=draw.firstIndex)throw std::runtime_error("base lightmap input/index range mismatch");
        for(uint32_t j=0;j<draw.indexCount;j+=3){
            const auto face=s.orderedTriangleFaces.at((draw.firstIndex-a.firstIndex+j)/3);const auto key=std::make_pair(&s,face);
            auto found=cached.find(key);size_t plane;
            if(found==cached.end()){
                auto decoded=SourceBspBaseLightmapCandidate::prepare(a.snapshot,face,material.source.syntax,SourceBaseLightmapUnitsCandidate::TexLightLinear);
                for(const auto& p:decoded->pixels())for(size_t k=0;k<3;++k)if(!std::isfinite(p[k])||p[k]<0||double(p[k])*std::max(1.,double(options.modulation[k]))>double(std::numeric_limits<float>::max())/4.)throw std::runtime_error("base lightmap arithmetic headroom");
                plane=next.planes.size();next.planes.push_back(std::move(decoded));cached.emplace(key,plane);
            }else plane=found->second;
            for(size_t k=0;k<3;++k){const auto global=u.indices.at(size_t(draw.firstIndex)+j+k);
                if(global<a.firstVertex||uint64_t(global)>=uint64_t(a.firstVertex)+a.vertexCount)throw std::runtime_error("base lightmap vertex window");
                const auto local=global-a.firstVertex;const auto& provenance=u.vertexShading.at(global);const auto& source=s.vertexLuxels.at(local);
                if(provenance.origin!=SceneShadingOriginCandidate::WorldGeometry||provenance.model!=a.model||provenance.sourceVertex!=local||source.faceIndex!=face)throw std::runtime_error("base lightmap world provenance mismatch");
                for(size_t axis=0;axis<2;++axis){const auto value=source.luxel[axis];if(!std::isfinite(value)||std::abs(value)>8192)throw std::runtime_error("base lightmap luxel range");next.luxels[global][axis]=float(value);}
            }
            auto split=draw;split.firstIndex+=j;split.indexCount=3;
            if(!output.empty()&&output.back().plane==plane&&output.back().draw.model==draw.model&&output.back().draw.range==draw.range&&uint64_t(output.back().draw.firstIndex)+output.back().draw.indexCount==split.firstIndex)output.back().draw.indexCount+=3;
            else output.push_back({split,plane});
        }
    }};append(u.opaque,next.opaque,false);append(u.translucent,next.translucent,true);return next;
}
