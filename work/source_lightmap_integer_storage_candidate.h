#pragma once
// Independently authored non-bump integer-HDR storage, not shader exposure or
// negotiated GPU texture behavior. Integer output, not FP sticky flags, is emulated.
#include <array>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cstddef>

namespace source_lightmap_integer_detail {
inline std::uint16_t quantize(float value,unsigned scaleExponent)noexcept{
    std::uint32_t bits;std::memcpy(&bits,&value,4);const auto magnitude=bits&0x7fffffffu;
    // Original unordered comparisons select the upper clamp, regardless of sign.
    if(magnitude>0x7f800000u)return 65535;
    if(bits&0x80000000u)return 0;
    const int exponent=int(magnitude>>23)-127;
    if(exponent>=16-int(scaleExponent))return 65535;
    if(exponent< -int(scaleExponent))return 0;
    return std::uint16_t(((magnitude&0x7fffffu)|0x800000u)>>(23-int(scaleExponent)-exponent));
}
}
inline std::uint16_t sourceLightmapIntegerRgbCandidate(float value)noexcept{
    return source_lightmap_integer_detail::quantize(value,12);
}
inline std::uint16_t sourceLightmapIntegerAlphaCandidate(float value)noexcept{
    return source_lightmap_integer_detail::quantize(value,16);
}
struct SourceLightmapIntegerStorageRegionCandidate {
    std::uint32_t width=0,height=0,x=0,y=0,rowStride=0;
};
inline bool sourcePackLightmapIntegerCandidate(std::vector<std::uint8_t>& destination,
    const std::vector<std::array<float,4>>& pixels,const SourceLightmapIntegerStorageRegionCandidate& region){
    if(!region.width||!region.height||!region.rowStride||region.rowStride>65535||
       std::uint64_t(region.width)*region.height!=pixels.size()||pixels.size()>16384)return false;
    const auto rowEnd=(std::uint64_t(region.x)+region.width)*8;
    if(rowEnd>region.rowStride)return false;
    const auto end=(std::uint64_t(region.y)+region.height-1)*region.rowStride+rowEnd;
    if(end>destination.size())return false;
    auto staged=destination;
    for(std::size_t y=0;y<region.height;++y)for(std::size_t x=0;x<region.width;++x){
        const auto at=(std::uint64_t(region.y)+y)*region.rowStride+(std::uint64_t(region.x)+x)*8;
        const auto& pixel=pixels[y*region.width+x];
        for(std::size_t channel=0;channel<4;++channel){const auto encoded=channel==3?
            sourceLightmapIntegerAlphaCandidate(pixel[channel]):sourceLightmapIntegerRgbCandidate(pixel[channel]);
            staged[std::size_t(at)+channel*2]=std::uint8_t(encoded&255u);staged[std::size_t(at)+channel*2+1]=std::uint8_t(encoded>>8);
        }
    }
    destination.swap(staged);return true;
}
