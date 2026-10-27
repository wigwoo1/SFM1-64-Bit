#pragma once
#include "material_proxy_float_input_candidate.h"
#include "material_equals_proxy_candidate.h"
#include "material_sine_value_candidate.h"
#include "material_texture_transform_proxy_candidate.h"

class MaterialSineProxyCandidate:public MaterialProxyInstanceCandidate {
    MaterialProxyBindingCandidate result;
    int32_t component=-1;
    MaterialProxyFloatInputCandidate period,maximum,minimum,offset;
public:
    bool initialize(const MaterialProxyContextCandidate&context,const SourceMaterialPatchNodeCandidate&declaration)override{
        const auto*field=sourceMaterialPatchFindConstCandidate(declaration,"resultVar");if(!field||field->kind!=1||field->text.size()>255)return false;
        auto name=field->text;auto bracket=name.find('[');component=-1;
        if(bracket!=std::string::npos){component=int32_t(materialStringIntegerCandidate(name.substr(bracket+1)));name.resize(bracket);if(component>3)return false;}
        if(sourceMaterialPatchLowerCandidate(name)=="$alpha")return false; // Renderer flag/snapshot side effects remain unfinished.
        result=context.find(name);const auto value=result.read();if(!value||!materialStringDomainCandidate(value->value.numeric.typeAndMetadata))return false;
        return period.initialize(context,declaration,"sineperiod",0x3f800000)&&maximum.initialize(context,declaration,"sinemax",0x3f800000)&&minimum.initialize(context,declaration,"sinemin",0)&&offset.initialize(context,declaration,"timeOffset",0);
    }
    bool evaluate(MaterialProxyEvaluationCandidate&frame)override{
        const auto time=frame.currentTime();if(!time)return false;
        // Preserve actual getter order before the shared result assignment.
        auto o=offset.read(frame),hi=maximum.read(frame),lo=minimum.read(frame),p=period.read(frame);if(!o||!hi||!lo||!p)return false;
        return materialProxySetResultFloatCandidate(frame,result,component,materialSineValueCandidate({*time,*p,*lo,*hi,*o}));
    }
};
// Explicit opt-in only. No missing effect is silently replaced or skipped.
struct MaterialFunctionProxyFactoryCandidate:MaterialProxyFactoryCandidate {
    MaterialProxyInstanceCandidate*create(const std::string&name)override{
        if(name=="Sine")return new MaterialSineProxyCandidate;
        if(name=="Equals")return new MaterialEqualsProxyCandidate;
        if(name=="TextureTransform")return new MaterialTextureTransformProxyCandidate;
        return nullptr;
    }
    void destroy(MaterialProxyInstanceCandidate*p)noexcept override{delete p;}
};
