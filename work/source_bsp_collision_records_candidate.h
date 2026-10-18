#pragma once
#include "source_bsp_world_collision_data_candidate.h"
#include "source_box_classification_candidate.h"
#include "source_optimized_box_record_candidate.h"
struct SourceBspCollisionRecordCandidate {bool optimized=false;std::array<uint8_t,48>box{};};
// Caller supplies resolved runtime surface IDs indexed by texture-info index.
// Classification is native-derived; malformed selected geometry is rejected,
// never silently redirected through a collision kernel with different results.
inline std::vector<SourceBspCollisionRecordCandidate> sourceBspCollisionRecordsCandidate(
 const SourceBspWorldCollisionDataCandidate&data,const std::vector<uint32_t>&surfaceIds){
 if(data.brushes.size()>8192||data.planes.size()>65536||data.sides.size()>65536)throw std::runtime_error("collision runtime limits exceeded");
 std::vector<SourceBoxRecordPlaneCandidate>planes;for(const auto&p:data.planes)planes.push_back({p.normal,p.distance,p.type});
 std::vector<SourceBspCollisionRecordCandidate>records;records.reserve(data.brushes.size());
 for(const auto&brush:data.brushes){
  if(brush.first>65535||brush.count>=65535||brush.first>data.sides.size()||brush.count>data.sides.size()-brush.first)throw std::runtime_error("invalid runtime brush span");
  std::array<uint8_t,6>types{};if(brush.count==6)for(size_t i=0;i<6;++i)types[i]=data.planes.at(data.sides.at(brush.first+i).plane).type;
  SourceBspCollisionRecordCandidate record;record.optimized=sourceBoxClassificationCandidate(uint16_t(brush.count),types);
  if(record.optimized){
   if(brush.count!=6)throw std::runtime_error("unsafe zero-side optimized brush");
   std::array<SourceBoxRecordSideCandidate,6>sides{};unsigned faces=0;
   for(size_t i=0;i<6;++i){const auto&s=data.sides.at(brush.first+i);const auto&p=data.planes.at(s.plane);const unsigned axis=p.type;
    if(axis>2||(p.normal[axis]!=1&&p.normal[axis]!=-1))throw std::runtime_error("invalid selected box normal");
    for(size_t j=0;j<3;++j)if(j!=axis&&p.normal[j]!=0)throw std::runtime_error("nonaxial selected box");
    const unsigned face=axis+(p.normal[axis]>0?3u:0u);if(faces&(1u<<face))throw std::runtime_error("duplicate selected box face");faces|=1u<<face;
    sides[i]={s.plane,s.texture,uint8_t(uint16_t(s.bevel)>>8)};
   }
   sourceOptimizedBoxRecordCandidate(planes,sides,surfaceIds,record.box);
   for(size_t axis=0;axis<3;++axis){float low,high;std::memcpy(&low,record.box.data()+4*axis,4);std::memcpy(&high,record.box.data()+16+4*axis,4);if(low>high)throw std::runtime_error("inverted selected box");}
  }
  records.push_back(record);
 }
 return records;
}
