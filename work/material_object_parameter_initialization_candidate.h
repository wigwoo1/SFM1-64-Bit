#pragma once
#include <memory>
#include <type_traits>
#include "material_fallback_parameters_candidate.h"
#include "builtin_error_material_seed_candidate.h"

// Value-owned parameter state, not a shader/GPU resource handle or IMaterial ABI.
struct MaterialObjectParameterStateCandidate {
    MaterialParameterOrchestrationCandidate material;
    std::vector<std::string>dependencies,patchFiles;
};
struct MaterialObjectParameterOptionsCandidate {
    SourceMaterialPatchNodeCandidate*externalSource=nullptr; // borrowed, never adopted
    SourceMaterialPatchNodeCandidate*externalPatch=nullptr; // borrowed accumulator
    SourceMaterialKeyValuesSourceCandidate provider;
    MaterialParameterCollectionEnvironmentCandidate environment;
    VertexLitInitSettingsCandidate settings;
    int32_t hardwareLevel=92;
};
enum class MaterialObjectParameterStatusCandidate {Refused,MissingRoot,Initialized,AlreadyInitialized};
struct MaterialObjectParameterAttemptCandidate {
    MaterialObjectParameterStatusCandidate status=MaterialObjectParameterStatusCandidate::Refused;
    uint32_t warnings=0,developerDiagnostics=0;
};
struct MaterialObjectParameterPlanCandidate {
    std::unique_ptr<MaterialObjectParameterStateCandidate>state;
    SourceMaterialPatchNodeCandidate finalRoot;
    SourceMaterialPatchNodeCandidate finalExternalPatch;
    bool updateExternalPatch=false;
    std::array<uint32_t,7>forwardValues{}; // alpha, color3, final tint3
    MaterialObjectParameterAttemptCandidate attempt;
};
inline bool materialObjectParameterTreeEqualCandidate(const SourceMaterialPatchNodeCandidate&a,const SourceMaterialPatchNodeCandidate&b){
    if(a.key!=b.key||a.kind!=b.kind||a.text!=b.text||a.bits!=b.bits||a.pointer!=b.pointer||a.children.size()!=b.children.size())return false;
    for(size_t i=0;i<a.children.size();++i)if(!materialObjectParameterTreeEqualCandidate(a.children[i],b.children[i]))return false;return true;
}
inline bool materialObjectParameterPrepareCandidate(MaterialObjectParameterPlanCandidate&out,
        const std::string&name,uint16_t flags,const SourceMaterialPatchNodeCandidate*stored,
        const MaterialObjectParameterOptionsCandidate&options){
    // Store wins over external input. Filename-loaded roots are temporary and
    // must not become the object's persistent supplied-source declaration.
    const auto*root=stored?stored:options.externalSource;
    SourceMaterialPatchNodeCandidate loaded;
    SourceMaterialPatchFilesResultCandidate file;
    auto provider=options.provider;
    if(!provider)provider=[](const std::string&)->std::optional<std::string>{return {};};
    if(!root){
        file=sourceMaterialPatchFilesCandidate(name,provider,(flags&0x20)!=0,true);
        if(file.status==SourceMaterialPatchFileLoadStatusCandidate::RootUnavailable){out.attempt={MaterialObjectParameterStatusCandidate::MissingRoot,1,0};return false;}
        if(file.document.roots.size()!=1||file.status==SourceMaterialPatchFileLoadStatusCandidate::RetainRootName)return false;
        loaded=std::move(file.document.roots.front());root=&loaded;
    }
    // Proxy factory/instances are not owned yet. Refuse rather than silently
    // accepting and dropping a declaration that requests live proxy behavior.
    auto proxies=[](auto&&self,const SourceMaterialPatchNodeCandidate&n)->bool{if(sourceMaterialPatchLowerCandidate(n.key)=="proxies"&&!n.children.empty())return true;for(const auto&c:n.children)if(self(self,c))return true;return false;};
    if(proxies(proxies,*root))return false;
    auto seed=builtinErrorMaterialSeedCandidate();MaterialFallbackParametersCandidate prepared;
    bool loadedFallback=false;auto fallbackProvider=[&](const std::string&path){auto value=provider(path);if(value&&!value->empty())loadedFallback=true;return value;};
    if(!materialFallbackParametersCandidate(prepared,*root,seed.source,name,(flags&0x20)!=0,fallbackProvider,options.environment,options.settings,options.hardwareLevel))return false;
    if(proxies(proxies,prepared.root))return false;
    MaterialObjectParameterPlanCandidate plan;plan.state=std::make_unique<MaterialObjectParameterStateCandidate>();
    plan.state->material=std::move(prepared.material);plan.finalRoot=std::move(prepared.root);
    if(!stored&&options.externalSource&&options.externalPatch&&loadedFallback){plan.finalExternalPatch=std::move(prepared.aggregate);plan.updateExternalPatch=true;}
    plan.state->dependencies=std::move(file.dependencies);plan.state->dependencies.insert(plan.state->dependencies.end(),prepared.dependencies.begin(),prepared.dependencies.end());
    plan.state->patchFiles=std::move(file.patchFiles);plan.state->patchFiles.insert(plan.state->patchFiles.end(),prepared.patchFiles.begin(),prepared.patchFiles.end());
    plan.state->material.parameters.warnings+=file.diagnosticCalls;plan.state->material.developerDiagnosticCalls+=file.developerDiagnosticCalls;
    const auto&v=plan.state->material.parameters.values;if(v.size()<=12||v.size()>255)return false;
    // Original GetFloat17C20/GetVec17CC0 copy the cached float projection,
    // including string variables whose constructor/setter already parsed it.
    // Resource/matrix getter behavior remains outside this checked family.
    for(size_t i:{4u,5u,11u})if(!materialStringDomainCandidate(v[i].value.numeric.typeAndMetadata))return false;
    plan.forwardValues[0]=v[5].value.numeric.vector[0];
    auto multiply=[](uint32_t a,uint32_t b){float x,y;std::memcpy(&x,&a,4);std::memcpy(&y,&b,4);volatile float product=x*y;uint32_t result;float f=product;std::memcpy(&result,&f,4);return result;};
    for(size_t i=0;i<3;++i){
        const auto color=v[4].value.numeric.vector[i];plan.forwardValues[1+i]=color;
        auto tint=multiply(color,v[11].value.numeric.vector[i]);
        if((options.environment.condition.srgbReturn&255u)&&(v[12].value.numeric.typeAndMetadata&15)==2)tint=multiply(tint,v[12].value.numeric.vector[i]);
        plan.forwardValues[4+i]=tint;
    }
    plan.attempt={MaterialObjectParameterStatusCandidate::Initialized,plan.state->material.parameters.warnings,plan.state->material.developerDiagnosticCalls};out=std::move(plan);return true;
}
