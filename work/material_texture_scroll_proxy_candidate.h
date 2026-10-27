#pragma once
#include "material_proxy_float_input_candidate.h"
#include "material_texture_scroll_value_candidate.h"

// TextureScroll ignores FloatInput Init's missing-name failure. The original
// stored binding still points at the shared missing variable. Other proxies
// keep their existing failure policy; this wrapper models this caller only.
class MaterialTextureScrollInputCandidate {
    MaterialProxyFloatInputCandidate ordinary;
    bool shared=false;
    int32_t component=-1;
public:
    bool initialize(const MaterialProxyContextCandidate&context,const SourceMaterialPatchNodeCandidate&declaration,const char*key,uint32_t fallback){
        MaterialTextureScrollInputCandidate next;
        if(next.ordinary.initialize(context,declaration,key,fallback)){*this=std::move(next);return true;}
        const auto*p=sourceMaterialPatchFindConstCandidate(declaration,key);if(!p||p->kind!=1||p->text.size()>4095)return false;
        auto name=p->text.substr(0,p->text.find('\0'));const auto bracket=name.find('[');
        if(bracket!=std::string::npos){if(bracket>=255)return false;name.resize(std::min<size_t>(255,name.size()));next.component=int32_t(materialStringIntegerCandidate(name.substr(bracket+1)));name.resize(bracket);}
        if(context.find(name).valid())return false;next.shared=true;*this=std::move(next);return true;
    }
    std::optional<uint32_t>read(const MaterialProxyEvaluationCandidate&frame)const{
        if(!shared)return ordinary.read(frame);
        if(component>=0){const auto count=frame.sharedMissingVariableCount();if(!count||*count>7)return {};if(uint32_t(component)>=*count)return 0;if(*count>4)return {};}
        const auto cache=frame.sharedMissingVariableCache();if(!cache)return {};
        auto value=cache->at(component<0?0:size_t(component));if((value&0x7fffffffu)>0x7f800000u)value|=0x400000u;return value;
    }
};

class MaterialTextureScrollProxyCandidate:public MaterialProxyInstanceCandidate {
    MaterialProxyBindingCandidate result;
    MaterialTextureScrollInputCandidate rate,angle,scale;
public:
    bool initialize(const MaterialProxyContextCandidate&context,const SourceMaterialPatchNodeCandidate&declaration)override{
        const auto*p=sourceMaterialPatchFindConstCandidate(declaration,"textureScrollVar");if(!p||p->kind!=1||p->text.size()>4095)return false;
        // This caller performs a literal FindVar; it does not use basic-result
        // component suffix parsing or its alpha-specific initialization flag.
        const auto name=p->text.substr(0,p->text.find('\0'));result=context.find(name);const auto value=result.read();
        if(!value||(!materialStringDomainCandidate(value->value.numeric.typeAndMetadata)&&!((value->value.numeric.typeAndMetadata&15u)==7&&value->matrix)))return false;
        return rate.initialize(context,declaration,"textureScrollRate",0x3f800000)&&angle.initialize(context,declaration,"textureScrollAngle",0)&&scale.initialize(context,declaration,"textureScale",0x3f800000);
    }
    bool evaluate(MaterialProxyEvaluationCandidate&frame)override{
        const auto r=rate.read(frame),a=angle.read(frame),s=scale.read(frame);const auto time=frame.currentTime();if(!r||!a||!s||!time)return false;
        const auto current=frame.read(result);if(!current)return false;
        const auto value=materialTextureScrollValueCandidate({*time,*r,*a,*s,current->value.numeric.typeAndMetadata});
        if(value[0]==9){MaterialMatrixMathCandidate matrix;std::copy_n(value.begin()+1,16,matrix.begin());return frame.setMatrix(result,matrix);}
        return frame.setVector3(result,{value[1],value[2],value[3]});
    }
};
