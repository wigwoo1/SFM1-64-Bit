#pragma once
#include "material_parameter_orchestration_candidate.h"
#include "source_material_patch_files_candidate.h"
#include <set>

struct MaterialFallbackParametersCandidate{
    MaterialParameterOrchestrationCandidate material;
    SourceMaterialPatchNodeCandidate root,aggregate;
    std::vector<std::string>dependencies,patchFiles;
};
inline bool materialFallbackParametersCandidate(MaterialFallbackParametersCandidate&out,
        const SourceMaterialPatchNodeCandidate&source,const SourceMaterialPatchNodeCandidate&error,
        const std::string&ownerName,bool prefixAlreadyProvided,const SourceMaterialKeyValuesSourceCandidate&provider,
        const MaterialParameterCollectionEnvironmentCandidate&environment={},const VertexLitInitSettingsCandidate&settings={},int32_t hardwareLevel=92){
    try{
        if(!provider)return false;
        for(unsigned char c:ownerName)if(c<32||c>126)return false;
        size_t errorNodes=0;sourceMaterialPatchFileTreeValidateCandidate(error,0,errorNodes);if(errorNodes>4096)return false;
        MaterialFallbackParametersCandidate result;result.aggregate.key="patchAccumulator";
        MaterialParameterFallbackServicesCandidate services;services.ownerName=ownerName;services.errorTree=error;sourceMaterialPatchFileAssignedTreeCandidate(services.errorTree);
        std::set<std::string>requests;size_t totalBytes=0,totalReads=0;
        services.load=[&](const std::string&name,SourceMaterialPatchNodeCandidate&loaded){
            const auto path=sourceMaterialKeyValuesPathCandidate((prefixAlreadyProvided?std::string():std::string("materials/"))+name+".vmt");
            if(path.size()>=260||!requests.insert(path).second)return MaterialParameterReloadStatusCandidate::Refused;
            auto first=provider(path);result.dependencies.push_back(path);++totalReads;
            if(!first)return MaterialParameterReloadStatusCandidate::Missing;
            bool initial=true;
            auto read=[&](const std::string&p)->std::optional<std::string>{
                std::optional<std::string>v;
                if(initial&&p==path){initial=false;v=std::move(first);}else{if(++totalReads>256)throw std::runtime_error("Fallback total reads");result.dependencies.push_back(p);v=provider(p);}
                if(v){if(v->size()>(1u<<26)-totalBytes)throw std::runtime_error("Fallback total bytes");totalBytes+=v->size();}return v;
            };
            auto file=sourceMaterialPatchFilesCandidate(name,read,prefixAlreadyProvided);
            // Original100069A8 clears the supplied accumulator on every
            // successful file parse, before checking whether its root is patch.
            // A missing root returns before this point and retains old state.
            result.aggregate=std::move(file.aggregate);
            result.patchFiles.insert(result.patchFiles.end(),file.patchFiles.begin(),file.patchFiles.end());
            if(file.document.roots.size()!=1)return MaterialParameterReloadStatusCandidate::Refused;
            loaded=std::move(file.document.roots.front());return MaterialParameterReloadStatusCandidate::Loaded;
        };
        if(!materialParameterOrchestrationCandidate(result.material,source,environment,settings,hardwareLevel,&services,&result.root))return false;
        out=std::move(result);return true;
    }catch(const std::runtime_error&){return false;}
}
