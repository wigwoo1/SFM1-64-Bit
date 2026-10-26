#pragma once
#include "vertexlit_concrete_init_candidate.h"
#include "shader_fallback_decision_candidate.h"
#include "material_parameter_flags_candidate.h"
#include "source_material_keyvalues_files_candidate.h"

// Read-only, pre-texture parameter orchestration. Supported concrete initializer
// schemas are explicit; known unsupported shaders and external fallback VMTs
// refuse instead of substituting a shader or flattening file/block behavior.
struct MaterialParameterOrchestrationCandidate {
    MaterialParameterCollectionCandidate parameters;
    std::string shader;
    uint32_t selectedNode=0;
    std::vector<std::string>lookups;
};
inline bool materialParameterOrchestrationCandidate(MaterialParameterOrchestrationCandidate&out,
        const SourceMaterialPatchNodeCandidate&root,const MaterialParameterCollectionEnvironmentCandidate&environment,
        const VertexLitInitSettingsCandidate&settings,int32_t hardwareLevel){
    using Node=SourceMaterialPatchNodeCandidate;
    std::vector<const Node*>nodes;bool valid=true;
    auto flatten=[&](auto&&self,const Node&n,size_t depth)->void{
        if(depth>16||nodes.size()>=4096||n.key.empty()||n.key.size()>255||n.key.find('/')!=std::string::npos||n.text.size()>4095||!(n.kind<=3||n.kind==7)){valid=false;return;}
        for(unsigned char c:n.key)if(c<32||c>126)valid=false;
        for(unsigned char c:n.text)if(!c||c>126)valid=false;
        if(sourceMaterialPatchLowerCandidate(n.key)=="$fallbackmaterial"&&(n.kind!=1||!n.text.empty()))valid=false;
        nodes.push_back(&n);for(const auto&child:n.children)self(self,child,depth+1);
    };flatten(flatten,root,0);if(!valid||!settings.vertexLit)return false;
    auto index=[&](const Node*p){for(size_t i=0;i<nodes.size();++i)if(nodes[i]==p)return uint32_t(i);throw std::runtime_error("Node identity");};
    auto find=[&](const Node&n,const std::string&name)->const Node*{const auto k=sourceMaterialPatchLowerCandidate(name);for(const auto&c:n.children)if(sourceMaterialPatchLowerCandidate(c.key)==k)return &c;return nullptr;};
    auto block=[&](const std::string&shader)->const Node*{
        auto match=[&](const std::string&condition)->const Node*{if(auto*p=find(root,condition))return p;return find(root,shader+"_"+condition);};
        const int32_t gpu=environment.condition.gpu;
        for(auto condition:std::vector<std::pair<bool,const char*>>{
            {gpu<1,"GPU<1"},{gpu<2,"GPU<2"},{gpu>=1,"GPU>=1"},{gpu>=2,"GPU>=2"},
            {hardwareLevel<90,"<DX90"},{hardwareLevel<95,"<DX95"},{hardwareLevel<92,"<DX90_20b"},
            {hardwareLevel>=92,">=DX90_20b"},{hardwareLevel<=90,"<=DX90"},{hardwareLevel>=90,">=DX90"},{hardwareLevel>90,">DX90"}})
            if(condition.first)if(auto*p=match(condition.second))return p;
        if(environment.condition.hdr){if(auto*p=match("hdr_dx9"))return p;if(auto*p=match("hdr"))return p;}
        else if(auto*p=match("ldr"))return p;
        if(environment.condition.srgbReturn&255u)if(auto*p=match("srgb"))return p;
        if(hardwareLevel>=90)if(auto*p=match("dx9"))return p;
        return nullptr;
    };
    auto rows=[](const Node&node){std::vector<MaterialParameterInputCandidate>v;for(const auto&c:node.children)v.push_back({c.key,c.text,c.kind,c.bits});return v;};
    const auto base=rows(root);const Node*overrides=nullptr,*selected=&root;bool model=false;
    std::string name=root.key;MaterialParameterOrchestrationCandidate result;uint32_t warnings=0;
    for(unsigned pass=0;pass<16;++pass){
        result.lookups.push_back(name);std::string canonical;
        for(const auto&e:shaderFallbackEntriesCandidate())if(sourceMaterialPatchLowerCandidate(std::string(e.name))==sourceMaterialPatchLowerCandidate(name)){canonical=e.name;break;}
        if(canonical.empty()){if(environment.percentGateReturn&255u)++warnings;name="Wireframe_DX9";result.lookups.push_back(name);canonical=name;}
        if(canonical!="VertexLitGeneric"&&canonical!="Wireframe"&&canonical!="Wireframe_DX9")return false;
        bool newBlock=false;if(!overrides){overrides=block(name);if(overrides){selected=overrides;newBlock=true;}}
        const bool vertex=canonical=="VertexLitGeneric";
        std::vector<std::string>names;auto types=vertexLitParameterTypesCandidate();
        const size_t declared=vertex?125:canonical=="Wireframe"?13:15;
        for(size_t i=0;i<(vertex?125u:13u);++i)names.emplace_back(vertexLitParameterNamesCandidate()[i]);
        if(declared==15){names.push_back("$DISPLACEMENTMAP");names.push_back("$DISPLACEMENTWRINKLE");}
        types.resize(declared);if(declared==15){types[13]=0;types[14]=8;}
        auto env=environment;env.model=model;MaterialParameterCollectionCandidate values;const auto over=overrides?rows(*overrides):std::vector<MaterialParameterInputCandidate>{};
        if(!materialParameterCollectionCandidate(values,names,materialParameterFlagNamesCandidate(),base,overrides?&over:nullptr,env)||values.values.size()>255)return false;
        std::vector<MaterialParameterCompletedValueCandidate>input;for(const auto&v:values.values)input.push_back({v,0});
        MaterialParameterCompletionCandidate completed;
        if(vertex){VertexLitConcreteCompletionCandidate v;if(!vertexLitConcreteCompletionCandidate(v,input,types,settings,hardwareLevel))return false;completed=std::move(v.completed);values.warnings+=v.warnings;}
        else if(!materialParameterCompletionCandidate(completed,input,types,[&](auto&v){
            if(declared==13)return true; // Original alias InitParams is RET8.
            auto integer=[&](size_t i){return v[i].value.value.numeric.integer;};
            auto set=[&](size_t i,uint32_t n){return materialParameterCompletionSetCandidate(v[i],0,{n,0,0,0});};
            uint32_t flags=integer(0);
            if(!(integer(1)&0x80000000)&&settings.forceVertexFog)flags|=0x80000000;
            // This registered family leaves all shared info fields undeclared
            // except displacement slots; generic InitParams has no textures.
            flags&=~(0x40u|0x100000u|0x400000u);
            return set(2,integer(2)|0x40000)&&set(0,flags|2u|0x4000u|0x10000000u);
        }))return false;
        values.values.clear();for(auto&v:completed.values)values.values.push_back(std::move(v.value));warnings+=values.warnings;
        if(canonical!="Wireframe"){
            values.warnings=warnings;result.parameters=std::move(values);result.shader=canonical;result.selectedNode=index(selected);out=std::move(result);return true;
        }
        model=(values.values[0].value.numeric.integer&0x800)!=0;
        name="Wireframe_DX9";
        if(!newBlock){overrides=find(root,name);if(overrides)selected=overrides;}
        // Values/resources are released before the next collection. No old
        // parameter array is reused across shader/schema changes.
    }
    return false;
}
struct SourceMaterialParameterPipelineCandidate{
    MaterialParameterOrchestrationCandidate material;
    std::vector<std::string>dependencies;
};
inline bool sourceMaterialParameterPipelineCandidate(SourceMaterialParameterPipelineCandidate&out,
        const std::string&path,const SourceMaterialKeyValuesSourceCandidate&provider,
        const MaterialParameterCollectionEnvironmentCandidate&environment,const VertexLitInitSettingsCandidate&settings,int32_t hardware){
    try{
        auto file=sourceMaterialKeyValuesFilesCandidate(path,provider);
        if(file.document.roots.empty()||sourceMaterialPatchLowerCandidate(file.document.roots[0].key)=="patch")return false;
        SourceMaterialParameterPipelineCandidate result;result.dependencies=std::move(file.dependencies);
        if(!materialParameterOrchestrationCandidate(result.material,file.document.roots[0],environment,settings,hardware))return false;
        out=std::move(result);return true;
    }catch(const std::runtime_error&){return false;}
}
