#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <optional>
#include <vector>

struct SourceSkyLightCandidate {
    std::array<float,3> intensity;
    std::array<float,3> towardLight;
};

// Present but empty means the supported table contains no sky emitters.
// Unknown BSP versions, compressed tables and invalid records stay unavailable.
inline std::optional<std::vector<SourceSkyLightCandidate>> readSourceHdrSkyLightsCandidate(
    const std::vector<uint8_t>& bytes){
    if(bytes.size()<1036 || std::memcmp(bytes.data(),"VBSP",4)!=0)return {};
    auto word=[&](size_t at){
        uint32_t value;
        std::memcpy(&value,bytes.data()+at,sizeof(value));
        return value;
    };
    constexpr size_t header=8+54*16, stride=88;
    if(word(4)!=20 || word(header+8)!=0 || word(header+12)!=0)return {};
    const size_t offset=word(header), length=word(header+4);
    if(offset>bytes.size() || length>bytes.size()-offset || length%stride)return {};
    std::vector<SourceSkyLightCandidate> result;
    for(size_t at=offset;at<offset+length;at+=stride){
        if(word(at+40)!=3)continue;
        SourceSkyLightCandidate light;
        std::memcpy(light.intensity.data(),bytes.data()+at+12,12);
        std::memcpy(light.towardLight.data(),bytes.data()+at+24,12);
        for(size_t axis=0;axis<3;++axis){
            if(!std::isfinite(light.intensity[axis]) || !std::isfinite(light.towardLight[axis]))return {};
            light.towardLight[axis]=-light.towardLight[axis];
        }
        result.push_back(light);
    }
    return result;
}
