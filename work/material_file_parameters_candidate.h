#pragma once
#include "source_material_patch_files_candidate.h"
#include "material_parameter_orchestration_candidate.h"

// Value-owned composition of the verified file/patch loader and parameter loop.
// External fallback materials and unimplemented concrete shader schemas still
// refuse in the loop. Nothing is published to out until both stages succeed.
struct MaterialFileParametersCandidate {
    SourceMaterialPatchFilesResultCandidate source;
    MaterialParameterOrchestrationCandidate material;
};
inline bool materialFileParametersCandidate(MaterialFileParametersCandidate&out,
        const std::string&name,const SourceMaterialKeyValuesSourceCandidate&provider,
        const MaterialParameterCollectionEnvironmentCandidate&environment,
        const VertexLitInitSettingsCandidate&settings,int32_t hardwareLevel,
        bool prefixAlreadyProvided=false){
    try{
        MaterialFileParametersCandidate result;
        result.source=sourceMaterialPatchFilesCandidate(name,provider,prefixAlreadyProvided);
        if(result.source.document.roots.empty()||!materialParameterOrchestrationCandidate(
                result.material,result.source.document.roots.front(),environment,settings,hardwareLevel))return false;
        out=std::move(result);return true;
    }catch(const std::runtime_error&){return false;}
}
