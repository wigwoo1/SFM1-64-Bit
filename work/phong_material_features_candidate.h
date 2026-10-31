#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
// Installed Phong draw helper 10037F40: EFFECTIVE typed values only.
// declared=false means the info index is -1, never "unknown" or "not loaded".
// A caller must resolve unknown state before constructing these inputs.
enum class PhongFeatureParamCandidate : size_t {
    Base,Compress,Stretch,Bump,BumpCompress,BumpStretch,EnvMap,SelfIllumFresnel,
    Phong,PhongWarp,DiffuseWarp,Detail,RimLight,Count
};
struct PhongFeatureValueCandidate {bool declared=false;uint8_t typeByte=0;uint32_t integer=0;};
struct PhongMaterialFeaturesInputsCandidate {
    std::array<PhongFeatureValueCandidate,size_t(PhongFeatureParamCandidate::Count)>values{};
    uint32_t flags=0,globalRimGate=0;
    bool flashlightOnly=false;
};
// Output order: base,baseWrinkle,bumpWrinkle,diffuseWarp,phong,phongWarp,
// detail,selfIllum,selfIllumFresnel,envMap,rimLight. No texture loading/routing.
inline std::array<uint8_t,11> phongMaterialFeaturesCandidate(const PhongMaterialFeaturesInputsCandidate&in){
    using P=PhongFeatureParamCandidate;
    auto value=[&](P p)->const PhongFeatureValueCandidate&{return in.values[size_t(p)];};
    // Native code unconditionally indexes EnvMap on the non-flashlight-only path.
    // Refuse an absent required index before producing a result.
    if(!in.flashlightOnly&&!value(P::EnvMap).declared)throw std::invalid_argument("Phong features require EnvMap index on normal path");
    auto texture=[&](P p){const auto&v=value(p);return v.declared&&(v.typeByte&15u)==3;};
    auto integer=[&](P p){const auto&v=value(p);return v.declared&&v.integer!=0;};
    const bool base=texture(P::Base),phong=integer(P::Phong),self=(in.flags&64u)!=0;
    return {uint8_t(base),uint8_t(base&&texture(P::Compress)&&texture(P::Stretch)),
        uint8_t(texture(P::Bump)&&texture(P::BumpCompress)&&texture(P::BumpStretch)),
        uint8_t(texture(P::DiffuseWarp)),uint8_t(phong),uint8_t(texture(P::PhongWarp)),
        uint8_t(texture(P::Detail)),uint8_t(self),uint8_t(self&&integer(P::SelfIllumFresnel)),
        uint8_t(!in.flashlightOnly&&texture(P::EnvMap)),uint8_t(in.globalRimGate!=0&&phong&&integer(P::RimLight))};
}
