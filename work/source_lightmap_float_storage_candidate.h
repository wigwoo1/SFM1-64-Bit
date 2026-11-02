#pragma once
// Independently authored bounded CPU float-lightmap storage. No exposure,
// format negotiation, native pixel-writer ABI or GPU sampling is implied.
#include <array>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cstddef>

enum class SourceLightmapFloatStorageCandidate { Rgba16Float, Rgba32Float };
struct SourceLightmapFloatStorageRegionCandidate {
    std::uint32_t width=0,height=0,x=0,y=0,rowStride=0;
};

inline std::uint16_t sourceLightmapHalfStorageCandidate(float value)noexcept{
    std::uint32_t bits;std::memcpy(&bits,&value,sizeof(bits));
    const auto sign=std::uint16_t((bits>>16)&0x8000u);
    const auto magnitude=bits&0x7fffffffu;
    // Store the observed saturating/truncating representation explicitly; do
    // not substitute hardware round-to-nearest half conversion.
    if(magnitude>0x7f800000u)return sign;
    if(magnitude>=0x477fe000u)return std::uint16_t(sign|0x7bffu);
    if(magnitude<0x33800000u)return sign;
    const int exponent=int(magnitude>>23)-127;
    const auto fraction=magnitude&0x7fffffu;
    if(exponent<-14){
        const unsigned shift=unsigned(-exponent-1);
        return std::uint16_t(sign|std::uint16_t((fraction|0x800000u)>>shift));
    }
    return std::uint16_t(sign|std::uint16_t((exponent+15)<<10)|std::uint16_t(fraction>>13));
}

inline bool sourcePackLightmapFloatCandidate(std::vector<std::uint8_t>& destination,
    const std::vector<std::array<float,4>>& pixels,
    const SourceLightmapFloatStorageRegionCandidate& region,
    SourceLightmapFloatStorageCandidate format){
    const std::uint64_t size=format==SourceLightmapFloatStorageCandidate::Rgba16Float?8:
        format==SourceLightmapFloatStorageCandidate::Rgba32Float?16:0;
    if(!size||!region.width||!region.height||!region.rowStride||region.rowStride>65535||
       std::uint64_t(region.width)*region.height!=pixels.size()||pixels.size()>16384)return false;
    const auto rowEnd=(std::uint64_t(region.x)+region.width)*size;
    if(rowEnd>region.rowStride)return false;
    const auto end=(std::uint64_t(region.y)+region.height-1)*region.rowStride+rowEnd;
    if(end>destination.size())return false;
    auto staged=destination;
    for(std::size_t y=0;y<region.height;++y)for(std::size_t x=0;x<region.width;++x){
        const auto offset=(std::uint64_t(region.y)+y)*region.rowStride+(std::uint64_t(region.x)+x)*size;
        const auto& pixel=pixels[y*region.width+x];
        for(std::size_t channel=0;channel<4;++channel){
            if(size==8){const auto v=sourceLightmapHalfStorageCandidate(pixel[channel]);
                staged[std::size_t(offset)+channel*2]=std::uint8_t(v&255u);
                staged[std::size_t(offset)+channel*2+1]=std::uint8_t(v>>8);
            }else{std::uint32_t v;std::memcpy(&v,&pixel[channel],4);
                for(unsigned byte=0;byte<4;++byte)staged[std::size_t(offset)+channel*4+byte]=std::uint8_t((v>>(byte*8))&255u);
            }
        }
    }
    destination.swap(staged);return true;
}
