#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <type_traits>

struct TextureFormatPolicyInputCandidate {
    uint32_t vtfFormat=0,textureFormat=0,textureFlags=0,compressionConfig=0;
};
struct TextureFormatPolicyServicesCandidate {
    std::function<std::optional<uint32_t>()>enabled,hdrType;
    std::function<std::optional<uint32_t>(uint32_t,uint32_t)>nearestFormat;
};
struct TextureFormatPolicyResultCandidate {
    uint32_t textureFormat=0,vtfFormat=0;
    bool changed=false,queriedNearest=false,queriedHdr=false;
    uint32_t requestedFormat=0,selectedFormat=0,conversionCount=0;
    std::array<uint32_t,2>conversions{};
};
// Measured installed format registry0..63. This is a request/decision plan,
// not a device capability query, pixel converter, texture load or GPU object.
inline bool textureFormatPolicyCandidate(const TextureFormatPolicyInputCandidate&input,
    const TextureFormatPolicyServicesCandidate&services,TextureFormatPolicyResultCandidate&output)noexcept{
    try{
        const auto v=input;const auto calls=services;
        if(v.vtfFormat>63||!calls.enabled)return false;
        const auto enabled=calls.enabled();if(!enabled)return false;
        TextureFormatPolicyResultCandidate next;next.textureFormat=v.textureFormat;next.vtfFormat=v.vtfFormat;
        if(*enabled&255u){
            const auto f=v.vtfFormat;
            const bool compressed=f==13||f==14||f==15||f==20||f==34||f==35||f==60||f==61||f==62;
            const bool special=f==22||f==23||f==24||f==25||f==26||f==27||f==29;
            next.requestedFormat=((v.compressionConfig&255u)&&compressed)||special?f:(v.textureFlags&0x3000u)?12u:f==5?5u:3u;
            if(!calls.nearestFormat)return false;
            const auto selected=calls.nearestFormat(next.requestedFormat,1);if(!selected||*selected>63)return false;
            next.queriedNearest=true;next.selectedFormat=*selected;
            if(v.textureFormat!=*selected){next.conversionCount=1;next.conversions[0]=*selected;next.textureFormat=*selected;next.vtfFormat=*selected;next.changed=true;}
            else{
                if(!calls.hdrType)return false;const auto hdr=calls.hdrType();if(!hdr)return false;next.queriedHdr=true;
                if(*hdr==1&&*selected==24){next.conversionCount=2;next.conversions={25,24};next.vtfFormat=24;next.changed=true;}
            }
        }
        static_assert(std::is_nothrow_move_assignable<TextureFormatPolicyResultCandidate>::value,"transaction publication");
        output=std::move(next);return true;
    }catch(...){return false;}
}
