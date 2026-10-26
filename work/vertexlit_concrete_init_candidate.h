#pragma once
#include "vertexlit_shared_init_candidate.h"
#include "material_parameter_completion_candidate.h"
#include "vertexlit_parameter_schema_candidate.h"

// Complete installed VertexLitGeneric InitParams body, including its optional
// effect passes. This still precedes texture/proxy loading and draw selection.
inline VertexLitInitRouteCandidate vertexLitConcreteInitCandidate(VertexLitInitAccessCandidate&v,const VertexLitInitSettingsCandidate&s){
    if(!s.vertexLit)throw std::runtime_error("Concrete VertexLitGeneric is vertex lit");
    const auto route=vertexLitSharedInitCandidate(v,s);
    using P=VertexLitInitParamCandidate;
    auto p=[](uint32_t i){return P(i);};
    auto df=[&](uint32_t i,float n){if(!v.defined(p(i)))v.setFloat(p(i),n);};
    auto dv=[&](uint32_t i,float a,float b,float c,float d){if(!v.defined(p(i)))v.setVector4(p(i),a,b,c,d);};
    auto enabled=[&](uint32_t i){if(!v.defined(p(i))){v.setInt(p(i),0);return false;}return v.integer(p(i))!=0;};
    if(enabled(73)){
        v.setInt(P::Flags2,v.integer(P::Flags2)|0x40000);
        v.setInt(P::Flags,v.integer(P::Flags)|0x800);
        v.setInt(P::Flags2,v.integer(P::Flags2)|0x40);
        df(74,0);df(76,0.1f);dv(75,1,1,1,1);
        if(!v.defined(p(29)))v.setInt(p(29),0);
    }
    if(enabled(65)){
        v.setInt(P::Flags2,v.integer(P::Flags2)|0x40000);
        dv(67,0.11f,0.124f,0,0);df(68,0);dv(70,1,1,1,0);df(72,0);
    }
    if(enabled(77)){
        v.setInt(P::Flags2,v.integer(P::Flags2)|0x40000);
        if(!v.defined(p(83)))v.setString(p(83),"env_cubemap");
        df(84,1.5f);df(85,0);
        for(uint32_t i=86;i<=89;++i)dv(i,0,0,0,0.0001f);
        dv(90,0,0,0,0);df(91,0.3f);df(92,0.42f);dv(93,1,1,1,0);
        df(94,1);df(95,0.66f);df(96,1);df(72,0);
    }
    return route;
}

// Owned adapter uses the verified real-variable value representation, rather
// than the earlier test-only simplified setter states. Unsupported GetString
// conversions refuse transactionally; string albedo names are supported.
struct VertexLitConcreteValueAccessCandidate final:VertexLitInitAccessCandidate {
    std::vector<MaterialParameterCompletedValueCandidate>&values;
    int32_t hardware=92;uint32_t warnings=0;
    explicit VertexLitConcreteValueAccessCandidate(std::vector<MaterialParameterCompletedValueCandidate>&v):values(v){}
    auto&at(P p){return values.at(size_t(p));}
    const auto&at(P p)const{return values.at(size_t(p));}
    size_t parameterCount()const override{return 125;}
    bool defined(P p)override{return (at(p).value.value.numeric.typeAndMetadata&15)!=6;}
    uint32_t integer(P p)const override{return at(p).value.value.numeric.integer;}
    uint8_t typeByte(P p)const override{return uint8_t(at(p).value.value.numeric.typeAndMetadata);}
    std::string_view string(P p)override{
        auto&v=at(p).value;if((v.value.numeric.typeAndMetadata&15)!=1)throw std::runtime_error("Albedo GetString conversion not yet verified");
        return v.value.text;
    }
    void setInt(P p,uint32_t n)override{if(!materialParameterCompletionSetCandidate(at(p),0,{n,0,0,0}))throw std::runtime_error("SetInt domain");}
    static uint32_t bits(float n){uint32_t b;std::memcpy(&b,&n,4);return b;}
    void setFloat(P p,float n)override{if(!materialParameterCompletionSetCandidate(at(p),1,{bits(n),0,0,0}))throw std::runtime_error("SetFloat domain");}
    void setVector3(P p,float a,float b,float c)override{if(!materialParameterCompletionSetCandidate(at(p),3,{bits(a),bits(b),bits(c),0}))throw std::runtime_error("SetVec3 domain");}
    void setVector4(P p,float a,float b,float c,float d)override{if(!materialParameterCompletionSetCandidate(at(p),4,{bits(a),bits(b),bits(c),bits(d)}))throw std::runtime_error("SetVec4 domain");}
    void unset(P p)override{if(!materialParameterCompletionSetCandidate(at(p),6))throw std::runtime_error("Unset domain");}
    void setString(P p,std::string_view text)override{
        auto next=at(p);const std::string copy(text);
        next.value.matrix.reset();next.value.matrixIdentity=false;next.nullResource=0;
        next.value.value.numeric.typeAndMetadata=(next.value.value.numeric.typeAndMetadata&~15u)|6;
        if(!materialStringSetCandidate(next.value.value,7,{},0,copy))throw std::runtime_error("SetString domain");
        at(p)=std::move(next);
    }
    int32_t hardwareLevel()override{return hardware;}
    void warning(unsigned code)override{if(code!=1&&code!=2)throw std::runtime_error("Warning domain");++warnings;}
};
struct VertexLitConcreteCompletionCandidate{
    MaterialParameterCompletionCandidate completed;
    VertexLitInitRouteCandidate route=VertexLitInitRouteCandidate::Generic;
    uint32_t warnings=0;
};
inline bool vertexLitConcreteCompletionCandidate(VertexLitConcreteCompletionCandidate&out,
        const std::vector<MaterialParameterCompletedValueCandidate>&input,const std::vector<uint32_t>&types,
        const VertexLitInitSettingsCandidate&s,int32_t hardware){
    if(types!=vertexLitParameterTypesCandidate()||input.size()<125||!s.vertexLit)return false;
    VertexLitConcreteCompletionCandidate result;
    try{
        if(!materialParameterCompletionCandidate(result.completed,input,types,[&](auto&values){
            VertexLitConcreteValueAccessCandidate access(values);access.hardware=hardware;
            result.route=vertexLitConcreteInitCandidate(access,s);result.warnings=access.warnings;return true;
        }))return false;
    }catch(const std::runtime_error&){return false;}
    out=std::move(result);return true;
}
