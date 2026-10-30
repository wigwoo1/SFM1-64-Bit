#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
// Original routing queries hardware virtual offset AC and tests only AL.
// DX95 eligibility alone does not establish this capability result.
inline unsigned phongPixelProgramVersionCandidate(uint32_t rawHardwareAc){return (rawHardwareAc&255u)?30u:20u;}
// Explicit effective draw inputs. These are NOT authored VMT values or defaults.
// Static input order: envmap,selfillum,selfillumFresnel,flashlight,flashlightOnly,
// diffuseWarp,phong,phongWarp,wrinkle,detail,detailMode,rim,shadowFilter,
// worldNormal,halfLambert,toolMode. Boolean source words admit byte values 0..255.
inline std::array<uint32_t,16> phongPixelStaticFieldsCandidate(unsigned version,const std::array<uint32_t,16>&x){
    if(version!=20&&version!=30)throw std::invalid_argument("Phong pixel version");
    for(unsigned i=0;i<16;++i)if(i!=10&&i!=12&&x[i]>255)throw std::invalid_argument("Phong source boolean byte");
    if(x[10]>6||x[12]>2)throw std::invalid_argument("Phong detail/filter domain");
    auto b=[&](unsigned i){return uint32_t(x[i]!=0);};
    std::array<uint32_t,16>f{b(0),b(1)&&!b(4),b(2)&&!b(4),b(3),b(5)&&b(6),b(7)&&b(6),b(8),x[10],b(9),b(11),x[12],0,0,b(14),0,0};
    if(version==30){f[12]=b(13);f[14]=b(15);}return f;
}
inline uint32_t phongPixelStaticArgumentCandidate(unsigned version,const std::array<uint32_t,16>&f){
    if(version!=20&&version!=30)throw std::invalid_argument("Phong pixel version");
    for(unsigned i=0;i<16;++i){
        const auto limit=i==7?6u:i==10?2u:1u;
        if(f[i]>limit)throw std::invalid_argument("Phong static field domain");
    }
    if(f[11]||f[15]||(version==20&&(f[12]||f[14])))throw std::invalid_argument("Phong reserved static field");
    uint32_t high=version==20?f[13]:f[12]+2*f[13]+4*f[14];
    uint32_t index=f[10]+3*high;
    index=f[9]+2*index;index=f[8]+2*index;index=f[7]+7*index;
    for(int i=6;i>=0;--i)index=f[size_t(i)]+2*index;
    return index*(version==20?80u:160u);
}
// Dynamic input order: raw signed fog mode,lightCount,waterFogToAlpha,
// depthToAlpha,flashlightShadows,uberlight,raw lightingPreviewMode, then zeros.
// PS30 suppresses water-fog/light count/flashlight shadows during preview mode.
// No claim that every encoded fixture can occur in a complete native draw.
inline std::array<uint32_t,16> phongPixelDynamicFieldsCandidate(unsigned version,const std::array<uint32_t,16>&x){
    if(version!=20&&version!=30)throw std::invalid_argument("Phong pixel version");
    if(x[1]>4)throw std::invalid_argument("Phong pixel light-count domain");
    for(unsigned i=2;i<6;++i)if(x[i]>255)throw std::invalid_argument("Phong dynamic boolean byte");
    for(unsigned i=7;i<16;++i)if(x[i])throw std::invalid_argument("Phong unused dynamic input");
    const bool suppress=version==30&&x[6]!=0;
    std::array<uint32_t,16>f{};f[0]=uint32_t(x[2]!=0&&!suppress);f[1]=uint32_t(x[0]==2);f[2]=suppress?0:x[1];
    f[3]=uint32_t(x[3]!=0);f[4]=uint32_t(x[4]!=0&&!suppress);if(version==30)f[5]=uint32_t(x[5]!=0);
    return f;
}
inline uint32_t phongPixelDynamicIndexCandidate(unsigned version,const std::array<uint32_t,16>&x){
    const auto f=phongPixelDynamicFieldsCandidate(version,x);
    return f[0]+2*f[1]+4*f[2]+20*f[3]+40*f[4]+80*f[5];
}
