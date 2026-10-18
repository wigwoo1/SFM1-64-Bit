#pragma once
#include "source_bsp_line_walk_candidate.h"
#include "source_leaf_brush_policy_candidate.h"
#include "source_brush_line_dispatch_candidate.h"
// Diagnostic brush-only world trace. Displacements are NOT included.
class SourceBspBrushOnlyTraceCandidate {
public:
 SourceBspWorldCollisionDataCandidate data;
 std::vector<SourceBspCollisionRecordCandidate>records;
 std::vector<SourceLineWalkPlaneCandidate>planes;
 std::vector<SourceLineWalkNodeCandidate>nodes;int32_t root=0;
 explicit SourceBspBrushOnlyTraceCandidate(const std::vector<uint8_t>&b,const std::vector<uint32_t>&surfaceIds):data(b),records(sourceBspCollisionRecordsCandidate(data,surfaceIds)){
  const SourceBspPointLeafCandidate checked(b);root=checked.worldRoot();for(const auto&p:data.planes)planes.push_back({p.normal,p.distance,p.type});
  auto word=[&](size_t p){uint32_t v;std::memcpy(&v,b.data()+p,4);return v;};auto child=[&](size_t p){int32_t v;std::memcpy(&v,b.data()+p,4);return v;};
  const size_t offset=word(8+5*16),count=word(8+5*16+4)/32;for(size_t i=0;i<count;++i){size_t p=offset+32*i;nodes.push_back({word(p),{child(p+4),child(p+8)}});}
 }
 void traceBrushesOnly(const std::array<float,3>&start,const std::array<float,3>&end,uint32_t mask,SourceOptimizedBoxLineStateCandidate&state)const{
  if(mask&0x2000)throw std::runtime_error("surface-exclusion masks not supported by brush-only adapter");
  auto result=state;std::vector<uint32_t>contents;for(const auto&b:data.brushes)contents.push_back(b.contents);std::vector<uint32_t>visited(data.brushes.size());
  sourceBspLineWalkCandidate(planes,nodes,root,uint32_t(data.leafBrushes.size()),start,end,0,1,result.fraction,[&](uint32_t leaf,float,float){
   sourceLeafBrushPolicyCandidate(data.leafBrushes.at(leaf),contents,visited,1,mask,result,[](uint32_t){return false;},[&](uint32_t brush,auto&s){sourceBrushLineDispatchCandidate(data,records,brush,start,end,s);});
  });state=result;
 }
};
