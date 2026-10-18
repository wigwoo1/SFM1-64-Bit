#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

using SourceDisplacementFaceInputCandidate=std::array<uint8_t,56>;

// Original loader scans face.dispinfo, rather than trusting info.mapFace.
// Last matching face wins; the stored face index is deliberately 16-bit.
inline std::vector<uint16_t>sourceDisplacementFaceMapCandidate(
 const std::vector<SourceDisplacementFaceInputCandidate>&faces,uint32_t displacementCount){
 if(displacementCount>65535||faces.size()>INT32_MAX)throw std::runtime_error("unsupported displacement face table size");
 std::vector<uint16_t>result(displacementCount,0xffff);
 for(size_t i=0;i<faces.size();++i){int16_t index;std::memcpy(&index,faces[i].data()+12,2);
  // Native assumes -1 or a nonnegative index; other negatives would write
  // before its mapping table. Reject malformed input instead of emulating UB.
  if(index< -1)throw std::runtime_error("invalid negative face displacement index");
  if(index>=0&&uint32_t(index)<displacementCount)result[size_t(index)]=uint16_t(i);
 }return result;
}

inline std::array<std::array<float,3>,4>sourceDisplacementFaceQuadCandidate(
 const SourceDisplacementFaceInputCandidate&face,const std::vector<std::array<float,3>>&vertices,
 const std::vector<std::array<uint16_t,2>>&edges,const std::vector<int32_t>&surfedges){
 int32_t first;int16_t count;std::memcpy(&first,face.data()+4,4);std::memcpy(&count,face.data()+8,2);
 if(count!=4||first<0||size_t(first)>surfedges.size()||surfedges.size()-size_t(first)<4)throw std::runtime_error("invalid displacement quad span");
 std::array<std::array<float,3>,4>result{};
 for(size_t i=0;i<4;++i){const auto se=surfedges[size_t(first)+i];const auto index=uint64_t(se<0?-int64_t(se):int64_t(se));
  if(index>=edges.size())throw std::runtime_error("displacement surfedge outside edge table");const auto vi=edges[size_t(index)][se<0?1:0];
  if(vi>=vertices.size())throw std::runtime_error("displacement edge vertex outside table");result[i]=vertices[vi];for(float v:result[i])if(!std::isfinite(v))throw std::runtime_error("nonfinite displacement quad vertex");
 }return result;
}
