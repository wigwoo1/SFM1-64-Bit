#pragma once
#include "scene_gpu_upload_candidate.h"

// The editor flattens opaque then translucent ranges for visibility/depth sorting.
// A list position is not the model-local range identity. Validate that this is
// still the same packet and retain all six original draw fields for recording.
// Return owned draw values; do not keep pointers into a replaceable frame.
template<class Ranges>
std::vector<SceneGpuDrawCandidate> sceneEditorDrawIdentitiesCandidate(
        const SceneGpuUploadCandidate& upload,const Ranges& flattened){
    if(upload.opaque.size()>SIZE_MAX-upload.translucent.size()||
       flattened.size()!=upload.opaque.size()+upload.translucent.size())
        throw std::runtime_error("editor retained draw coverage");
    std::vector<SceneGpuDrawCandidate> result;
    result.reserve(flattened.size());
    for(const auto* group:{&upload.opaque,&upload.translucent})for(const auto& draw:*group){
        const auto& range=flattened.at(result.size());
        if(range.firstIndex!=draw.firstIndex||range.indexCount!=draw.indexCount||
           range.materialIndex!=draw.materialSlot)
            throw std::runtime_error("editor flattened draw identity");
        if(draw.materialSlot>=upload.materials.size())
            throw std::runtime_error("editor retained material slot");
        const auto& address=upload.materials[draw.materialSlot];
        if(address.model!=draw.model||address.material!=draw.materialIndex)
            throw std::runtime_error("editor retained material identity");
        if(draw.firstIndex>upload.indices.size()||draw.indexCount>upload.indices.size()-draw.firstIndex||
           draw.indexCount%3)
            throw std::runtime_error("editor retained draw span");
        result.push_back(draw);
    }
    return result;
}
