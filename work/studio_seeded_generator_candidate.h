#pragma once
#include <array>
#include <cstdint>
struct StudioSeedBlockCandidate {
 std::array<float,220> values{};
 float* first=nullptr;
 float* second=nullptr;
 std::array<std::uint32_t,2> preservedTail{};
};
using StudioSeedStateCandidate=std::array<StudioSeedBlockCandidate,32>;
inline void studioInitializeSeedCandidate(StudioSeedStateCandidate& blocks,std::uint32_t seed){
 for(std::uint32_t i=0;i<32;++i){auto& b=blocks[i];b.first=&b.values[92];b.second=&b.values[216];std::uint32_t x=seed+i;
 for(auto& v:b.values){v=static_cast<float>(static_cast<double>(x>>16)*(1.0/65536));x=0xbb40e62du-x*0x44bf19d3u;}}
}
