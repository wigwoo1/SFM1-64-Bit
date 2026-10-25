#pragma once
#include "phong_material_init_candidate.h"
#include "shader_skin_material_predicate_candidate.h"
#include <array>
#include <cstddef>

// Installed VertexLitGeneric schema only. These are verified registry indices,
// not raw native pointers/offsets and not a schema for other shader families.
enum class VertexLitInitParamCandidate:uint32_t{
    Flags=0,FlagsDefined=1,Flags2=2,Base=6,Flashlight=9,Albedo=13,SelfIllumTint=16,
    DetailScale=19,EnvMap=20,EnvFrame=21,EnvMask=22,EnvMaskFrame=23,EnvTint=25,
    Bump=26,BumpFrame=29,EnvContrast=31,EnvSaturation=32,SelfIllumFresnel=34,
    SelfIllumMinMax=35,SelfIllumMaskScale=36,AlphaTestReference=37,FlashlightNoLambert=38,
    LightWarp=43,Phong=48,BaseAlphaPhongMask=49,EnvFresnel=51,SelfIllumMask=52,
    BaseLuminancePhongMask=53,DetailBlendFactor=55,DetailTint=56,SeamlessScale=64,
    LinearWrite=98,ShaderSrgbRead360=99,AmbientOcclusion=100,DisplacementWrinkle=103,
    AllowDiffuse=106,EnvFresnelMinMax=107,BaseAlphaEnvMinMax=108,PhongDisableHalfLambert=109,
    TreeSway=110,TreeHeight=111,TreeStartHeight=112,TreeRadius=113,TreeStartRadius=114,
    TreeSpeed=115,TreeHighWind=116,TreeStrength=117,TreeScrumbleSpeed=118,
    TreeScrumbleStrength=119,TreeScrumbleFrequency=120,TreeFalloff=121,
    TreeScrumbleFalloff=122,TreeLerpStart=123,TreeLerpEnd=124
};
struct VertexLitInitAccessCandidate{
    using P=VertexLitInitParamCandidate;
    virtual ~VertexLitInitAccessCandidate()=default;
    virtual size_t parameterCount()const=0;
    virtual bool defined(P)=0;
    // Direct field reads, unlike the observable virtual queries/mutations.
    virtual uint32_t integer(P)const=0;
    virtual uint8_t typeByte(P)const=0;
    virtual std::string_view string(P)=0;
    virtual void setInt(P,uint32_t)=0;
    virtual void setFloat(P,float)=0;
    virtual void setString(P,std::string_view)=0;
    virtual void setVector3(P,float,float,float)=0;
    virtual void setVector4(P,float,float,float,float)=0;
    virtual void unset(P)=0;
    virtual int32_t hardwareLevel()=0;
    // 1: normal + base-alpha env mask; 2: normal + explicit env mask.
    virtual void warning(unsigned)=0;
};
struct VertexLitInitSettingsCandidate{
    uint32_t configFlagsAt34=0;
    int32_t matPhong=0,forceVertexFog=0;
    bool vertexLit=true,toolsSwitchPresent=false;
};
enum class VertexLitInitRouteCandidate:uint32_t{Generic=0,Phong=1};

struct VertexLitPhongAdapterCandidate final:PhongInitAccessCandidate{
    VertexLitInitAccessCandidate&v;
    explicit VertexLitPhongAdapterCandidate(VertexLitInitAccessCandidate&in):v(in){}
    static VertexLitInitParamCandidate map(Param p){
        static constexpr std::array<uint32_t,19>ids{0,2,9,6,13,26,20,43,48,35,51,100,103,34,49,53,99,106,109};
        return VertexLitInitParamCandidate(ids.at(size_t(p)));
    }
    bool declared(Param)const override{return true;} // Fixed registered schema.
    bool defined(Param p)override{return v.defined(map(p));}
    uint32_t integer(Param p)const override{return v.integer(map(p));}
    std::string_view string(Param p)override{return v.string(map(p));}
    void setInt(Param p,uint32_t n)override{v.setInt(map(p),n);}
    void setFloat(Param p,float n)override{v.setFloat(map(p),n);}
    void setString(Param p,std::string_view n)override{v.setString(map(p),n);}
    void setVector3(Param p,float a,float b,float c)override{v.setVector3(map(p),a,b,c);}
};
inline VertexLitInitRouteCandidate vertexLitSharedInitCandidate(VertexLitInitAccessCandidate&v,const VertexLitInitSettingsCandidate&s){
    using P=VertexLitInitParamCandidate;
    if(v.parameterCount()!=125)throw std::runtime_error("installed VertexLitGeneric schema required");
    const auto di=[&](P p,uint32_t value){if(!v.defined(p))v.setInt(p,value);};
    const auto df=[&](P p,float value){if(!v.defined(p))v.setFloat(p,value);};
    const auto dv3=[&](P p,float a,float b,float c){if(!v.defined(p))v.setVector3(p,a,b,c);};
    di(P::ShaderSrgbRead360,0);di(P::Phong,0);df(P::AlphaTestReference,0);
    // info+5C and info+14C are undeclared in the installed family mapping.
    di(P::FlashlightNoLambert,0);dv3(P::DetailTint,1,1,1);dv3(P::EnvTint,1,1,1);
    di(P::EnvFrame,0);di(P::BumpFrame,0);df(P::DetailBlendFactor,1);df(P::DetailScale,4);
    dv3(P::SelfIllumTint,1,1,1);
    if(v.defined(P::SelfIllumMask))v.setInt(P::Flags2,v.integer(P::Flags2)|0x400000);
    df(P::SelfIllumMaskScale,1);
    if(!(v.integer(P::FlagsDefined)&0x80000000)&&s.forceVertexFog)
        v.setInt(P::Flags,v.integer(P::Flags)|0x80000000);
    ShaderSkinMaterialInputsCandidate predicate;
    predicate.matPhong=s.matPhong;predicate.phong=int32_t(v.integer(P::Phong));
    predicate.diffuseWarpTypeByte=v.typeByte(P::LightWarp);
    predicate.baseAlphaPhongMask=int32_t(v.integer(P::BaseAlphaPhongMask));
    if(s.matPhong&&v.integer(P::Phong)&&(v.typeByte(P::LightWarp)&15)!=3&&v.integer(P::BaseAlphaPhongMask)!=1)
        predicate.bumpDefined=v.defined(P::Bump);
    else predicate.bumpDefined=false;
    if(shaderSkinMaterialPredicateCandidate(predicate).wantsSkin){
        if(v.hardwareLevel()>=92){
            VertexLitPhongAdapterCandidate adapter(v);
            phongMaterialInitCandidate(adapter,s.configFlagsAt34,s.toolsSwitchPresent);
            return VertexLitInitRouteCandidate::Phong;
        }
        v.setInt(P::Phong,0);
    }
    v.setString(P::Flashlight,"effects/flashlight001");
    if(!(s.configFlagsAt34&0x100)&&v.defined(P::Bump)&&v.defined(P::Albedo)&&v.defined(P::Base)){
        const auto name=v.string(P::Albedo);v.setString(P::Base,name);
    }
    v.setInt(P::Flags2,v.integer(P::Flags2)|0x40000);
    if(s.vertexLit)v.setInt(P::Flags2,v.integer(P::Flags2)|2);
    else v.setInt(P::Flags,v.integer(P::Flags)&~0x40u);
    di(P::EnvMaskFrame,0);df(P::EnvContrast,0);df(P::EnvSaturation,1);df(P::SeamlessScale,0);
    // info E8/EC/FC/118 have no registered variables in this family.
    if(!v.defined(P::Base)){
        v.setInt(P::Flags,v.integer(P::Flags)&~0x40u);
        v.setInt(P::Flags,v.integer(P::Flags)&~0x100000u);
    }
    if(v.integer(P::Flags)&0x10000)v.setInt(P::Flags,v.integer(P::Flags)|2);
    const bool bump=!(s.configFlagsAt34&0x100)&&v.defined(P::Bump);
    if(bump||(s.vertexLit&&v.defined(P::LightWarp)))v.setInt(P::Flags2,v.integer(P::Flags2)|0x40);
    else v.setInt(P::Flags,v.integer(P::Flags)&~0x400000u);
    const bool baseAlpha=(v.integer(P::Flags)&0x400000)!=0;
    if(baseAlpha){v.unset(P::EnvMask);v.setInt(P::Flags,v.integer(P::Flags)&~0x100000u);}
    if((v.integer(P::Flags)&0x100000)&&v.defined(P::Bump)&&!baseAlpha){v.warning(1);v.unset(P::EnvMap);}
    if(v.defined(P::EnvMask)&&v.defined(P::Bump)){
        v.unset(P::EnvMask);if(!baseAlpha){v.warning(2);v.unset(P::EnvMap);}
    }
    if((s.configFlagsAt34&0x80)&&v.defined(P::EnvMap)&&v.defined(P::Base))v.unset(P::EnvMap);
    // info A4/13C are undeclared for this family.
    di(P::LinearWrite,0);di(P::AllowDiffuse,1);
    if(!v.defined(P::EnvFresnelMinMax))v.setVector4(P::EnvFresnelMinMax,0,1,2,0);
    if(!v.defined(P::BaseAlphaEnvMinMax))v.setVector4(P::BaseAlphaEnvMinMax,1,0,1,0);
    di(P::TreeSway,0);df(P::TreeHeight,1000);df(P::TreeStartHeight,0.1f);
    df(P::TreeRadius,300);df(P::TreeStartRadius,0.2f);df(P::TreeSpeed,1);
    df(P::TreeHighWind,2);df(P::TreeStrength,10);df(P::TreeScrumbleSpeed,5);
    df(P::TreeScrumbleStrength,10);df(P::TreeScrumbleFrequency,12);df(P::TreeFalloff,1.5f);
    df(P::TreeScrumbleFalloff,1);df(P::TreeLerpStart,3);df(P::TreeLerpEnd,6);
    return VertexLitInitRouteCandidate::Generic;
}
