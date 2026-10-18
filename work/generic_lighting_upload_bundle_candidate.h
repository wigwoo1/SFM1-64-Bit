#pragma once
#include "generic_ambient_constants_candidate.h"
#include "generic_light_float_upload_candidate.h"
#include "generic_lighting_metadata_candidate.h"
struct GenericLightingUploadBundleCandidate {
    GenericLightingConstantsRecordCandidate record;
    NativeShaderCommandPlanCandidate floats;
    std::vector<VertexMetadataUploadCandidate> metadata;
};
// Prepare every range before device callbacks. Configuration and unused-light
// baseline remain explicit inputs; full original helper branch policy is partial.
inline GenericLightingUploadBundleCandidate genericLightingUploadBundleCandidate(
    const std::vector<uint8_t>& vertexShader,const GenericAmbientStateCandidate& ambient,
    const GenericLightArrayCandidate& lights,int32_t configuredCount,bool skipUnusedFill,
    const std::vector<uint32_t>& unusedWords,const GenericLightingConstantsRecordCandidate& previous){
    GenericLightingUploadBundleCandidate result;result.record=previous;
    result.record.update(ambient,lights);
    if(lights.count>0){
        const auto constants=d3d9ShaderConstantsCandidate(vertexShader);
        const D3d9ShaderConstantCandidate* lightInfo=nullptr;
        for(const auto& entry:constants)if(entry.name=="cLightInfo")lightInfo=&entry;
        if(!lightInfo||lightInfo->registerSet!=2||lightInfo->index!=27||lightInfo->count<uint32_t(lights.count)*5)
            throw std::invalid_argument("lighting shader light constant metadata");
    }
    result.floats=genericAmbientConstantsCandidate(vertexShader,ambient);
    auto lightPlan=genericLightFloatUploadCandidate(result.record,configuredCount,skipUnusedFill,unusedWords);
    result.floats.uploads.insert(result.floats.uploads.end(),lightPlan.uploads.begin(),lightPlan.uploads.end());
    result.metadata=genericLightingMetadataCandidate(result.record);
    return result;
}
