#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
struct StudioShaderColorSelectionCandidate {std::array<uint8_t,3> rgb;bool convert;uint32_t packed;};
// Only verified jumptable modes1..6. Mode0 has a separate path;
// mode7's earlier snapshot handling does not establish a valid table entry.
inline StudioShaderColorSelectionCandidate studioShaderColorSelectCandidate(uint32_t mode,std::array<uint8_t,3> live,uint32_t flag){
 bool eligible=true;
 switch(mode){
 case 1:case 3:live={128,128,128};break;
 case 2:live={0,0,0};eligible=false;break;
 case 4:case 6:break;
 case 5:live={255,255,255};eligible=false;break;
 default:throw std::invalid_argument("unverified shader color dispatch mode");
 }
 return {live,eligible&&!uint8_t(flag),0xff000000u|(uint32_t(live[0])<<16)|(uint32_t(live[1])<<8)|live[2]};
}
