#pragma once
#include <cstdint>
#include <initializer_list>
#include <stdexcept>
#include <string_view>

// Bounded installed Phong parameter stage (10037850), not VMT parsing,
// texture loading, shader routing or a full effective-material constructor.
enum class PhongInitParamCandidate : uint32_t {
    Flags, Flags2, FlashlightTexture, BaseTexture, Albedo, BumpMap, EnvMap,
    LightWarp, Phong, SelfIllumFresnelMinMaxExp, EnvMapFresnel, AmbientOcclusion,
    DisplacementWrinkle, SelfIllumFresnel, BaseAlphaPhongMask,
    BaseLuminancePhongMask, ShaderSrgbRead360, AllowDiffuseModulation,
    PhongDisableHalfLambert, Count
};
struct PhongInitAccessCandidate {
    using Param=PhongInitParamCandidate;
    virtual ~PhongInitAccessCandidate()=default;
    // Declared means an index exists; defined means the material variable
    // callback reports a value. Neither means unknown/unloaded.
    virtual bool declared(Param) const=0;
    virtual bool defined(Param)=0;
    virtual uint32_t integer(Param) const=0;
    virtual std::string_view string(Param)=0;
    virtual void setInt(Param,uint32_t)=0;
    virtual void setFloat(Param,float)=0;
    virtual void setString(Param,std::string_view)=0;
    virtual void setVector3(Param,float,float,float)=0;
};
inline void phongMaterialInitCandidate(PhongInitAccessCandidate&v,uint32_t configFlagsAt34,bool toolsSwitchPresent){
    using P=PhongInitParamCandidate;
    // Refuse missing required variables before any mutation. Optional absence
    // mirrors info index -1. config bit8 is explicit, not inferred from VMTs.
    for(auto p:{P::Flags,P::Flags2,P::FlashlightTexture,P::BaseTexture})
        if(!v.declared(p))throw std::runtime_error("Phong init required variable absent");
    v.setString(P::FlashlightTexture,"effects/flashlight001");
    if(v.declared(P::Albedo)&&!(configFlagsAt34&0x100)&&v.declared(P::BumpMap)&&
       v.defined(P::BumpMap)&&v.defined(P::Albedo)&&v.defined(P::BaseTexture)){
        const auto albedo=v.string(P::Albedo);v.setString(P::BaseTexture,albedo);
    }
    v.setInt(P::Flags2,v.integer(P::Flags2)|0x40000);
    v.setInt(P::Flags2,v.integer(P::Flags2)|2);
    if(!v.defined(P::BaseTexture))v.setInt(P::Flags,v.integer(P::Flags)&~0x100000u);
    if(v.integer(P::Flags)&0x10000)v.setInt(P::Flags,v.integer(P::Flags)|2);
    v.setInt(P::Flags2,v.integer(P::Flags2)|0x40);
    const bool bump=v.declared(P::BumpMap)&&!(configFlagsAt34&0x100)&&v.defined(P::BumpMap);
    const bool env=v.declared(P::EnvMap)&&v.defined(P::EnvMap);
    const bool warp=v.declared(P::LightWarp)&&v.defined(P::LightWarp);
    const bool phong=v.declared(P::Phong)&&v.defined(P::Phong);
    if(!bump&&!env&&!warp&&!phong)v.setInt(P::Flags,v.integer(P::Flags)&~0x400000u);
    if(v.declared(P::SelfIllumFresnelMinMaxExp)&&!v.defined(P::SelfIllumFresnelMinMaxExp))
        v.setVector3(P::SelfIllumFresnelMinMaxExp,0,1,1);
    const auto defaultFloat=[&](P p,float value){if(v.declared(p)&&!v.defined(p))v.setFloat(p,value);};
    const auto defaultInt=[&](P p,uint32_t value){if(v.declared(p)&&!v.defined(p))v.setInt(p,value);};
    defaultFloat(P::EnvMapFresnel,0);
    // Installed helper 10003120 caches whether CommandLine has "-tools".
    // Supply that state explicitly; it is not a constant or a VMT setting.
    defaultFloat(P::AmbientOcclusion,toolsSwitchPresent?1.0f:0.0f);
    defaultFloat(P::DisplacementWrinkle,0);
    defaultInt(P::SelfIllumFresnel,0);
    defaultInt(P::BaseAlphaPhongMask,0);
    defaultInt(P::BaseLuminancePhongMask,0);
    defaultInt(P::ShaderSrgbRead360,0);
    defaultInt(P::AllowDiffuseModulation,1);
    defaultInt(P::PhongDisableHalfLambert,0);
}
