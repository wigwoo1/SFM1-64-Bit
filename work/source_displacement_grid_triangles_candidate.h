#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

struct SourceDisplacementGridTriangleCandidate {
 std::array<uint16_t,3>vertices{};
 uint16_t tags=0;
};

// Original CoreDisp 1013DC30 -> 1013DB60 -> 1013D960/1013DA60.
// Original 10141C70 copies these indices into 8-byte triangle records without
// writing the final tag word. Retain raw tags, including unknown bits.
// This is collision/CoreDisp winding, not the preview renderer's convention.
inline std::vector<SourceDisplacementGridTriangleCandidate>
sourceDisplacementGridTrianglesCandidate(uint32_t power,const std::vector<uint16_t>&tags){
 if(power<1||power>4)throw std::runtime_error("displacement triangle power outside supported range");
 const uint32_t cells=1u<<power,width=cells+1,count=cells*cells*2;
 if(tags.size()!=count)throw std::runtime_error("displacement triangle tag count mismatch");
 std::vector<SourceDisplacementGridTriangleCandidate>triangles;
 triangles.reserve(count);
 auto append=[&](uint32_t a,uint32_t b,uint32_t c){triangles.push_back({{uint16_t(a),uint16_t(b),uint16_t(c)},tags[triangles.size()]});};
 for(uint32_t row=0;row<cells;++row)for(uint32_t column=0;column<cells;++column){
  const uint32_t index=row*width+column;
  if(index&1){
   append(index,index+width,index+1);
   append(index+1,index+width,index+width+1);
  }else{
   append(index,index+width,index+width+1);
   append(index,index+width+1,index+1);
  }
 }
 return triangles;
}
