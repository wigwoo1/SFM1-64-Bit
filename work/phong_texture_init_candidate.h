#pragma once
#include <cstdint>
#include <initializer_list>
#include <stdexcept>

// Complete installed 10037BD0 control flow plus the three guarded load wrappers.
// Effective definitions and service outcomes must be supplied by the caller.
// This does not resolve texture names, load VTFs or infer successful loading.
enum class PhongTextureParamCandidate : uint32_t {
    Flags,Flags2,Flashlight,Base,Compress,Stretch,Exponent,Phong,DiffuseWarp,
    PhongWarp,Detail,DetailMode,Bump,BumpCompress,BumpStretch,EnvMap,
    SelfIllumFresnel,SelfIllumMask,Displacement,Count
};
enum class PhongTextureLoadKindCandidate : uint32_t { Texture=0,Bump=1,Cube=2 };
struct PhongTextureInitAccessCandidate {
    using P=PhongTextureParamCandidate;
    virtual ~PhongTextureInitAccessCandidate()=default;
    virtual bool declared(P)const=0;
    virtual bool defined(P)=0;
    virtual uint32_t integer(P)const=0;
    virtual void setInteger(P,uint32_t)=0;
    // Explicit GetTextureValue + texture virtual offset18 query boundary.
    // The native stage dereferences the returned texture; unavailable is not false.
    virtual bool baseTextureQuery()=0;
    virtual uint32_t hdrQuery()=0;
    virtual void load(PhongTextureLoadKindCandidate,P,uint32_t flags)=0;
};
inline void phongTextureInitCandidate(PhongTextureInitAccessCandidate&v,uint32_t configFlagsAt34){
    using P=PhongTextureParamCandidate;using K=PhongTextureLoadKindCandidate;
    for(auto p:{P::Flags,P::Flags2,P::Base,P::EnvMap})
        if(!v.declared(p))throw std::invalid_argument("Phong texture init required variable absent");
    const auto load=[&](K k,P p,uint32_t flags){if(v.declared(p)&&v.defined(p))v.load(k,p,flags);};
    load(K::Texture,P::Flashlight,0x80000);
    bool baseQuery=false;
    if(v.defined(P::Base)){
        load(K::Texture,P::Base,0x80000);baseQuery=v.baseTextureQuery();
        if(v.declared(P::Compress)&&v.declared(P::Stretch)&&v.defined(P::Compress)&&v.defined(P::Stretch)){
            load(K::Texture,P::Compress,0x80000);load(K::Texture,P::Stretch,0x80000);
        }
    }
    const bool self=(v.integer(P::Flags)&64u)!=0;
    const bool mask=self&&(v.integer(P::Flags2)&0x400000u)!=0;
    if(!baseQuery){
        if(!(self&&v.declared(P::SelfIllumFresnel)&&v.integer(P::SelfIllumFresnel)!=0)&&!mask)
            v.setInteger(P::Flags,v.integer(P::Flags)&~64u);
        v.setInteger(P::Flags,v.integer(P::Flags)&~0x100000u);
    }
    for(auto p:{P::Exponent,P::DiffuseWarp,P::PhongWarp})
        if(v.declared(p)&&v.defined(p)&&v.declared(P::Phong)&&v.defined(P::Phong))load(K::Texture,p,0);
    if(v.declared(P::Detail)&&v.defined(P::Detail)){
        const auto mode=v.declared(P::DetailMode)?v.integer(P::DetailMode):0u;
        load(K::Texture,P::Detail,(mode==2||mode==3||mode==4)?0x80000:0);
    }
    if(!(configFlagsAt34&0x100u)&&v.declared(P::Bump)&&v.defined(P::Bump)){
        load(K::Bump,P::Bump,0);v.setInteger(P::Flags2,v.integer(P::Flags2)|16u);
        if(v.declared(P::BumpCompress)&&v.declared(P::BumpStretch)&&v.defined(P::BumpCompress)&&v.defined(P::BumpStretch)){
            load(K::Texture,P::BumpCompress,0);load(K::Texture,P::BumpStretch,0);
        }
    }
    if(v.defined(P::EnvMap)){const auto hdr=v.hdrQuery();load(K::Cube,P::EnvMap,hdr?0u:0x80000u);}
    if(mask)load(K::Texture,P::SelfIllumMask,0);
    if(v.declared(P::Displacement)&&v.defined(P::Displacement))load(K::Texture,P::Displacement,0);
}
