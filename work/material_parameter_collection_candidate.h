#pragma once
#include "material_parameter_condition_candidate.h"
#include "material_typed_dispatch_candidate.h"
#include <vector>

struct MaterialParameterInputCandidate {
    std::string name,text;
    uint32_t type=0,bits=0;
};
struct MaterialParameterCollectionEnvironmentCandidate {
    MaterialParameterConditionEnvironmentCandidate condition;
    // Raw return words for the original percent-name policy. The semantic
    // identities of these two external service methods remain unverified.
    uint32_t percentGateReturn=0,percentKeepReturn=0;
    int32_t percentOverride=0;
    bool model=false,numericSse2=true,trigSse2=true;
};
struct MaterialParameterCollectionCandidate {
    std::vector<MaterialTypedDispatchCandidate>values;
    uint32_t warnings=0;
};
inline bool materialParameterCollectionCandidate(MaterialParameterCollectionCandidate&out,
        const std::vector<std::string>&parameters,const std::vector<std::string>&flagNames,
        const std::vector<MaterialParameterInputCandidate>&base,
        const std::vector<MaterialParameterInputCandidate>*overrides,
        const MaterialParameterCollectionEnvironmentCandidate&env) {
    if(parameters.size()<4||parameters.size()>256||flagNames.size()>32||base.size()>512||(overrides&&overrides->size()>512))return false;
    auto cstr=[](std::string_view s){return s.substr(0,s.find('\0'));};
    auto lower=[](char c){return c>='A'&&c<='Z'?char(c-'A'+'a'):c;};
    auto equal=[&](std::string_view a,std::string_view b){
        if(a.size()!=b.size())return false;
        for(size_t i=0;i<a.size();++i)if(lower(a[i])!=lower(b[i]))return false;
        return true;
    };
    auto trimmedMatch=[&](std::string_view key,std::string_view descriptor){
        while(!key.empty()&&(key.front()==' '||key.front()=='\t'))key.remove_prefix(1);
        if(key.size()<descriptor.size()||!equal(key.substr(0,descriptor.size()),descriptor))return false;
        key.remove_prefix(descriptor.size());for(char c:key)if(c!=' '&&c!='\t')return false;return true;
    };
    for(const auto&s:parameters)if(s.size()>1023||cstr(s).empty())return false;
    for(const auto&s:flagNames)if(s.size()>1023)return false;
    auto valid=[&](const std::vector<MaterialParameterInputCandidate>&rows){
        for(const auto&n:rows){
            if(n.name.size()>1023||n.text.size()>4095||n.type>255||n.type==5)return false;
            const auto name=cstr(n.name);const auto question=name.find('?');
            if(question!=std::string_view::npos&&question>255)return false;
        }
        return true;
    };
    if(!valid(base)||(overrides&&!valid(*overrides)))return false;
    MaterialParameterCollectionCandidate result;result.values.resize(parameters.size());
    auto make=[&](std::string_view name,MaterialValueConstructionKindCandidate kind,uint32_t bits=0){
        MaterialTypedDispatchCandidate v;v.created=materialValueConstructCandidate(v.value,kind,{bits,0,0,0},0,{},env.numericSse2);
        v.value.numeric.typeAndMetadata&=0xffffu;v.name.assign(cstr(name));for(auto&c:v.name)c=lower(c);return v;
    };
    const uint32_t model=env.model?0x800u:0u;
    result.values[0]=make("$flags",MaterialValueConstructionKindCandidate::Integer,model);
    result.values[1]=make("$flags_defined",MaterialValueConstructionKindCandidate::Integer,model);
    result.values[2]=make("$flags2",MaterialValueConstructionKindCandidate::Integer);
    result.values[3]=make("$flags_defined2",MaterialValueConstructionKindCandidate::Integer);
    std::array<bool,256>fromOverride{};uint32_t baseFlags=0,overrideFlags=0;
    auto setInteger=[&](MaterialTypedDispatchCandidate&v,uint32_t bits){
        if((v.value.numeric.typeAndMetadata&15)==4&&v.value.numeric.integer==bits)return;
        v.value.numeric.integer=bits;v.value.numeric.vector.fill(materialNumericIntFloatCandidate(bits));
        v.value.numeric.typeAndMetadata=(v.value.numeric.typeAndMetadata&0xfff4u)|4u;
        v.value.text.clear();v.matrix.reset();v.matrixIdentity=false;
    };
    auto consume=[&](const std::vector<MaterialParameterInputCandidate>&rows,bool overridePass){
        for(const auto&n:rows){
            const auto name=cstr(n.name);MaterialParameterConditionCandidate condition;
            if(!materialParameterConditionCandidate(condition,name,env.condition))return false;
            result.warnings+=condition.unrecognized?1u:0u;if(condition.skip)continue;
            if(!name.empty()&&name.front()=='%'&&(env.percentGateReturn&255u)&&!(env.percentKeepReturn&255u)&&!env.percentOverride)continue;
            auto suffix=name;const auto question=suffix.find('?');if(question!=std::string_view::npos)suffix.remove_prefix(question+1);
            uint32_t flag=0;
            for(size_t i=0;i<flagNames.size();++i){const auto token=cstr(flagNames[i]);if(token.empty())break;if(trimmedMatch(suffix,token)){flag=uint32_t(1)<<i;break;}}
            if(flag){
                auto&seen=overridePass?overrideFlags:baseFlags;
                if(seen&flag){++result.warnings;continue;}
                if(overrideFlags&flag)continue;
                seen|=flag;
                const uint32_t bits=n.type==1?materialStringIntegerCandidate(cstr(n.text)):n.type==3?materialNumericTruncateCandidate(n.bits):n.type==7?0:n.bits;
                auto&flags=result.values[0];auto&defined=result.values[1];
                setInteger(flags,bits?flags.value.numeric.integer|flag:flags.value.numeric.integer&~flag);
                setInteger(defined,defined.value.numeric.integer|flag);continue;
            }
            int index=-1;
            for(size_t i=parameters.size();i-->0;)if(trimmedMatch(suffix,cstr(parameters[i]))){index=int(i);break;}
            if(index>=0){
                if(result.values[size_t(index)].created&&!condition.conditional){
                    if(fromOverride[size_t(index)]&&!overridePass)continue;
                    ++result.warnings;continue;
                }
            }else{
                for(size_t i=parameters.size();i<result.values.size();++i)if(equal(name,result.values[i].name)){index=int(i);break;}
                if(index>=0){if(fromOverride[size_t(index)]&&!overridePass)continue;++result.warnings;continue;}
            }
            MaterialTypedDispatchCandidate value;
            if(!materialTypedDispatchCandidate(value,n.type,n.bits,name,cstr(n.text),env.numericSse2,env.trigSse2))return false;
            result.warnings+=value.warnings;value.warnings=0;if(!value.created)continue;
            if(index<0){if(result.values.size()==256)return false;index=int(result.values.size());result.values.push_back({});}
            result.values[size_t(index)]=std::move(value);if(overridePass)fromOverride[size_t(index)]=true;
        }
        return true;
    };
    // Original early-exit: an explicitly supplied empty override root prevents
    // traversal of the base root. An absent override root does not.
    if(overrides){if(!overrides->empty()&&(!consume(*overrides,true)||!consume(base,false)))return false;}
    else if(!consume(base,false))return false;
    for(size_t i=0;i<parameters.size();++i)if(!result.values[i].created)result.values[i]=make(parameters[i],MaterialValueConstructionKindCandidate::Undefined);
    out=std::move(result);return true;
}
