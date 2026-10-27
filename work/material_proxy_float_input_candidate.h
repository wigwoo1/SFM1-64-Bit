#pragma once
#include "material_proxy_evaluation_candidate.h"
#include "material_matrix_scan_candidate.h"

// Shared input reader used by Sine and other function proxies. No clock or
// sine approximation is supplied here. Bindings remain authenticated copies.
class MaterialProxyFloatInputCandidate {
    MaterialProxyBindingCandidate variable;
    uint32_t literal=0;
    int32_t component=-1;
    bool initialized=false,bound=false;
    static uint32_t returnedFloat(uint32_t bits){
        // Original input getter returns through x87 and its caller stores f32.
        if((bits&0x7f800000u)==0x7f800000u&&(bits&0x7fffffu))bits|=0x400000u;
        return bits;
    }
public:
    bool initialize(const MaterialProxyContextCandidate&context,const SourceMaterialPatchNodeCandidate&declaration,
                    const std::string&key,uint32_t defaultBits){
        MaterialProxyFloatInputCandidate next;next.literal=defaultBits;
        const auto*input=sourceMaterialPatchFindConstCandidate(declaration,key);
        if(input){
            switch(input->kind){
            case 0:next.literal=0;break;
            case 2:next.literal=materialNumericIntFloatCandidate(input->bits);break;
            case 3:next.literal=input->bits;break;
            case 1:{
                auto text=input->text.substr(0,input->text.find('\0'));if(text.size()>4095)return false;
                // This checked scanner's first assignment has the same %f
                // grammar/conversion. Later format assignments are ignored.
                std::array<uint32_t,16>words{};
                if(materialMatrixScanCandidate("[ "+text,false,words)>0){next.literal=words[0];break;}
                const auto bracket=text.find('[');
                if(bracket!=std::string::npos){
                    // Native copies255 characters before strchr/strtol. A
                    // bracket outside that copy would be unsafe; refuse it.
                    if(bracket>=255)return false;
                    text.resize(std::min<size_t>(255,text.size()));
                    next.component=int32_t(materialStringIntegerCandidate(text.substr(bracket+1)));text.resize(bracket);
                }
                next.variable=context.find(text);if(!next.variable.valid())return false;next.bound=true;break;
            }
            default:return false; // Pointer/wide/color/uint64 KV getters unproved.
            }
        }
        next.initialized=true;*this=std::move(next);return true;
    }
    std::optional<uint32_t>read(const MaterialProxyEvaluationCandidate&frame)const{
        if(!initialized)return {};
        if(!bound)return returnedFloat(literal);
        const auto value=frame.read(variable);if(!value)return {};
        if(component<0)return returnedFloat(value->value.numeric.vector[0]);
        const auto count=(value->value.numeric.typeAndMetadata>>4)&7u;
        if(uint32_t(component)>=count)return 0;
        if(count>4)return {}; // Native four-element scratch requires this bound.
        return returnedFloat(value->value.numeric.vector[size_t(component)]);
    }
};
