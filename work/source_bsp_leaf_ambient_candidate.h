#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

enum class SourceBspAmbientEncodingCandidate {Hdr,Ldr};
struct SourceBspAmbientLeafCandidate {
 std::array<int16_t,3>mins{},maxs{};
 uint16_t count=0,first=0;
 std::optional<uint32_t>sampleOwner;
};
struct SourceBspAmbientSampleCandidate {
 std::array<std::array<uint8_t,4>,6>cube{};
 std::array<uint8_t,3>position{};
};
// Explicit dataset selection. No native sample interpolation or shader binding.
class SourceBspLeafAmbientCandidate {
public:
 SourceBspAmbientEncodingCandidate encoding;
 std::vector<SourceBspAmbientLeafCandidate>leaves;
 std::vector<SourceBspAmbientSampleCandidate>samples;
 explicit SourceBspLeafAmbientCandidate(const std::vector<uint8_t>&bytes,SourceBspAmbientEncodingCandidate selected):encoding(selected){
  if(selected!=SourceBspAmbientEncodingCandidate::Hdr&&selected!=SourceBspAmbientEncodingCandidate::Ldr)throw std::runtime_error("explicit ambient dataset required");
  auto u16=[&](size_t p){return uint16_t(uint16_t(bytes.at(p))|(uint16_t(bytes.at(p+1))<<8));};
  auto u32=[&](size_t p){return uint32_t(u16(p))|(uint32_t(u16(p+2))<<16);};
  auto s16=[&](size_t p){const auto value=u16(p);return int16_t(value<32768?int32_t(value):int32_t(value)-65536);};
  if(bytes.size()<1036||u32(0)!=0x50534256||u32(4)!=20)throw std::runtime_error("ambient reader requires BSP20 header");
  auto lump=[&](unsigned id,uint32_t version,size_t stride){
   size_t p=8+id*16;uint32_t offset=u32(p),length=u32(p+4);
   if(offset>INT32_MAX||length>INT32_MAX||uint64_t(offset)+length>bytes.size()||(length&&offset<1036)||u32(p+8)!=version||u32(p+12)!=0||length%stride)
    throw std::runtime_error("unsupported or invalid BSP ambient lump "+std::to_string(id));
   return std::array<size_t,2>{offset,length/stride};
  };
  const auto leaf=lump(10,1,32),index=lump(selected==SourceBspAmbientEncodingCandidate::Hdr?51:52,0,4),lighting=lump(selected==SourceBspAmbientEncodingCandidate::Hdr?55:56,1,28);
  if(leaf[1]==0||index[1]!=leaf[1]||leaf[1]>65536)throw std::runtime_error("ambient index/leaf coverage");
  leaves.resize(leaf[1]);samples.resize(lighting[1]);
  for(size_t i=0;i<leaves.size();++i){auto&out=leaves[i];for(size_t axis=0;axis<3;++axis){out.mins[axis]=s16(leaf[0]+i*32+8+axis*2);out.maxs[axis]=s16(leaf[0]+i*32+14+axis*2);if(out.mins[axis]>out.maxs[axis])throw std::runtime_error("ambient leaf bounds reversed");}
   out.count=u16(index[0]+i*4);out.first=u16(index[0]+i*4+2);
   if(out.count){if(size_t(out.first)>samples.size()||size_t(out.count)>samples.size()-out.first)throw std::runtime_error("ambient sample span outside dataset");}
   else if(out.first>=leaves.size())throw std::runtime_error("ambient redirect outside leaf table");
  }
  for(size_t i=0;i<samples.size();++i){auto&out=samples[i];size_t p=lighting[0]+i*28;for(size_t face=0;face<6;++face)for(size_t channel=0;channel<4;++channel)out.cube[face][channel]=bytes[p+face*4+channel];for(size_t axis=0;axis<3;++axis)out.position[axis]=bytes[p+24+axis];}
  // Resolve redirects once in linear time. A self-reference with no samples is
  // the compiler's possible no-neighbor result; multi-leaf cycles are invalid.
  std::vector<uint8_t>state(leaves.size());
  for(uint32_t first=0;first<leaves.size();++first){if(state[first]==2)continue;std::vector<uint32_t>path;uint32_t cursor=first;std::optional<uint32_t>owner;
   for(;;){if(state[cursor]==2){owner=leaves[cursor].sampleOwner;break;}if(state[cursor]==1)throw std::runtime_error("ambient redirect cycle");state[cursor]=1;path.push_back(cursor);
    const auto&entry=leaves[cursor];if(entry.count){owner=cursor;break;}if(entry.first==cursor)break;cursor=entry.first;
   }
   for(auto entry:path){leaves[entry].sampleOwner=owner;state[entry]=2;}
  }
 }
 std::array<double,3>samplePosition(uint32_t leaf,size_t localSample)const{
  const auto owner=leaves.at(leaf).sampleOwner;if(!owner)throw std::runtime_error("leaf has no ambient samples");const auto&entry=leaves.at(*owner);if(localSample>=entry.count)throw std::runtime_error("ambient local sample outside span");
  const auto&sample=samples.at(size_t(entry.first)+localSample);std::array<double,3>result{};
  for(size_t axis=0;axis<3;++axis)result[axis]=double(entry.mins[axis])+(double(entry.maxs[axis])-entry.mins[axis])*(double(sample.position[axis])/255.0);return result;
 }
 // SDK ColorRGBExp32ToVector convention, NOT normalized lightmap texel units.
 // Doubles preserve the full stored exponent range without float overflow.
 static std::array<double,3>rgbExpVector(const std::array<uint8_t,4>&value){const int exponent=value[3]<128?value[3]:int(value[3])-256;return {std::ldexp(double(value[0]),exponent),std::ldexp(double(value[1]),exponent),std::ldexp(double(value[2]),exponent)};}
};
