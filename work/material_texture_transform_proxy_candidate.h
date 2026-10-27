#pragma once
#include "material_proxy_evaluation_candidate.h"
#include "material_texture_transform_matrix_candidate.h"

class MaterialTextureTransformProxyCandidate:public MaterialProxyInstanceCandidate {
    struct Input {
        bool configured=false,missing=false;
        MaterialProxyBindingCandidate binding;
        bool initialize(const MaterialProxyContextCandidate&context,const SourceMaterialPatchNodeCandidate&declaration,const char*key){
            const auto*p=sourceMaterialPatchFindConstCandidate(declaration,key);
            if(!p)return true;
            if(p->kind!=1||p->text.size()>4095)return false; // Other GetString conversions require separate proof.
            const auto name=p->text.substr(0,p->text.find('\0'));if(name.empty())return true;
            configured=true;binding=context.find(name);missing=!binding.valid();return true;
        }
        std::optional<std::array<uint32_t,4>>read(const MaterialProxyEvaluationCandidate&frame)const{
            if(missing)return frame.sharedMissingVariableCache();
            auto value=frame.read(binding);if(!value)return {};return value->value.numeric.vector;
        }
    };
    MaterialProxyBindingCandidate result;
    Input center,scale,rotation,translation;
public:
    bool initialize(const MaterialProxyContextCandidate&context,const SourceMaterialPatchNodeCandidate&declaration)override{
        if(!center.initialize(context,declaration,"centerVar")||!scale.initialize(context,declaration,"scaleVar")||!rotation.initialize(context,declaration,"rotateVar")||!translation.initialize(context,declaration,"translateVar"))return false;
        const auto*p=sourceMaterialPatchFindConstCandidate(declaration,"resultVar");if(!p||p->kind!=1||p->text.size()>4095)return false;
        auto name=p->text.substr(0,p->text.find('\0'));const auto bracket=name.find('[');
        if(bracket!=std::string::npos){if(bracket>=255)return false;name.resize(bracket);}
        // The actual OnBind always sets a full matrix, ignoring the parsed
        // result component. Alpha flags/snapshots remain outside this gate.
        if(sourceMaterialPatchLowerCandidate(name)=="$alpha")return false;
        result=context.find(name);auto value=result.read();return value&&(materialStringDomainCandidate(value->value.numeric.typeAndMetadata)||(value->value.numeric.typeAndMetadata&15u)==7);
    }
    bool evaluate(MaterialProxyEvaluationCandidate&frame)override{
        MaterialTextureTransformInputsCandidate input;
        if(center.configured){auto v=center.read(frame);if(!v)return false;input.center=std::array<uint32_t,2>{(*v)[0],(*v)[1]};}
        if(scale.configured){auto v=scale.read(frame);if(!v)return false;input.scale=std::array<uint32_t,2>{(*v)[0],(*v)[1]};}
        if(rotation.configured){auto v=rotation.read(frame);if(!v)return false;input.rotation=(*v)[0];}
        if(translation.configured){auto v=translation.read(frame);if(!v)return false;input.translation=std::array<uint32_t,2>{(*v)[0],(*v)[1]};}
        return frame.setMatrix(result,materialTextureTransformMatrixCandidate(input));
    }
};
