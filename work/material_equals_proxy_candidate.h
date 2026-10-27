#pragma once
#include "material_proxy_evaluation_candidate.h"

class MaterialEqualsProxyCandidate:public MaterialProxyInstanceCandidate {
    MaterialProxyBindingCandidate result,first,second;
    int32_t component=-1;
public:
    bool initialize(const MaterialProxyContextCandidate&context,const SourceMaterialPatchNodeCandidate&declaration)override{
        auto field=[&](const char*key,std::string&text){auto*p=sourceMaterialPatchFindConstCandidate(declaration,key);if(!p){text.clear();return true;}if(p->kind!=1||p->text.size()>255)return false;text=p->text;return true;};
        std::string output,source,optional;if(!field("resultVar",output)||!field("srcVar1",source)||!field("srcVar2",optional))return false;
        component=-1;auto bracket=output.find('[');
        if(bracket!=std::string::npos){component=int32_t(materialStringIntegerCandidate(output.substr(bracket+1)));output.resize(bracket);if(component>3)return false;}
        // Original alpha-result Init changes a material flag and recomputes
        // renderer snapshots. That side effect is outside this CPU-only gate.
        if(sourceMaterialPatchLowerCandidate(output)=="$alpha")return false;
        result=context.find(output);first=context.find(source);second=optional.empty()?MaterialProxyBindingCandidate{}:context.find(optional);
        return result.valid()&&first.valid()&&(optional.empty()||second.valid());
    }
    bool evaluate(MaterialProxyEvaluationCandidate&frame)override{
        auto output=frame.read(result),source=frame.read(first);if(!output||!source)return false;
        const auto rawType=output->value.numeric.typeAndMetadata&15;auto type=rawType;
        uint32_t count=(output->value.numeric.typeAndMetadata>>4)&7u;
        if(type==2&&component>=0)type=0;
        else if(type==6){
            type=source->value.numeric.typeAndMetadata&15;count=(source->value.numeric.typeAndMetadata>>4)&7u;
            if(type==6&&second.valid()){auto other=frame.read(second);if(!other)return false;type=other->value.numeric.typeAndMetadata&15;count=(other->value.numeric.typeAndMetadata>>4)&7u;}
        }
        switch(type){
        case 0:return materialProxySetResultFloatCandidate(frame,result,component,source->value.numeric.vector[0]);
        case 2:return frame.setVector(result,source->value.numeric.vector,count);
        case 4:return frame.setInt(result,source->value.numeric.integer);
        default:return true; // Actual Equals has no write for string/resource/undefined cases.
        }
    }
};
// Exact registered spelling only until dictionary case behavior is measured.
// Other built-ins, including the complete pilot material, stay unsupported.
struct MaterialEqualsProxyFactoryCandidate:MaterialProxyFactoryCandidate {
    MaterialProxyInstanceCandidate*create(const std::string&name)override{return name=="Equals"?new MaterialEqualsProxyCandidate:nullptr;}
    void destroy(MaterialProxyInstanceCandidate*p)noexcept override{delete p;}
};
