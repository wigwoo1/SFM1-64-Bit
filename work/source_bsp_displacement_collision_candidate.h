#pragma once
#include "source_bsp_world_collision_data_candidate.h"
#include "source_displacement_face_input_candidate.h"
#include "source_displacement_load_policy_candidate.h"
#include "source_displacement_core_geometry_candidate.h"
#include "source_displacement_runtime_mesh_candidate.h"

// Standalone BSP-backed collision meshes. No material binding, renderer or
// stationary/hull path is supplied. Unsupported inactive/non-quad records
// fail explicitly instead of exposing partial/uninitialized runtime data.
class SourceBspDisplacementCollisionCandidate {
public:
 uint32_t selectedFaceLump=7;
 std::vector<SourceDisplacementRuntimeMeshCandidate>meshes;
 explicit SourceBspDisplacementCollisionCandidate(const std::vector<uint8_t>&b,int32_t hdrType=0){
  const SourceBspWorldCollisionDataCandidate checked(b);(void)checked;
  auto u32=[&](size_t p){uint32_t v;std::memcpy(&v,b.data()+p,4);return v;};
  auto scalar=[&](size_t p){float v;std::memcpy(&v,b.data()+p,4);return v;};
  auto lump=[&](uint32_t id,size_t stride,uint32_t maxVersion=0){const size_t h=8+16*id,o=u32(h),n=u32(h+4);if(o>b.size()||n>b.size()-o||(n&&o<1036)||n%stride||n>INT32_MAX||u32(h+8)>maxVersion||u32(h+12))throw std::runtime_error("unsupported/corrupt displacement collision lump");return std::array<size_t,2>{o,n/stride};};
  int32_t hdrBytes;std::memcpy(&hdrBytes,b.data()+8+58*16+4,4);selectedFaceLump=sourceDisplacementFaceLumpCandidate(hdrType,hdrBytes);
  const auto fi=lump(selectedFaceLump,56,1),vi=lump(3,12),ei=lump(12,4),si=lump(13,4),di=lump(26,176),dv=lump(33,20),dt=lump(48,2);
  if(di[1]>65535)throw std::runtime_error("displacement collision count outside mapping range");
  std::vector<SourceDisplacementFaceInputCandidate>faces(fi[1]);if(!faces.empty())std::memcpy(faces.data(),b.data()+fi[0],faces.size()*56);
  std::vector<std::array<float,3>>vertices(vi[1]);if(!vertices.empty())std::memcpy(vertices.data(),b.data()+vi[0],vertices.size()*12);
  std::vector<std::array<uint16_t,2>>edges(ei[1]);if(!edges.empty())std::memcpy(edges.data(),b.data()+ei[0],edges.size()*4);
  std::vector<int32_t>surfedges(si[1]);if(!surfedges.empty())std::memcpy(surfedges.data(),b.data()+si[0],surfedges.size()*4);
  const auto mapping=sourceDisplacementFaceMapCandidate(faces,uint32_t(di[1]));uint32_t vertexCursor=0,triangleCursor=0;
  for(size_t i=0;i<di[1];++i){const auto face=mapping[i];if(face==0xffff)throw std::runtime_error("inactive displacement collision record unsupported");if(face>=faces.size())throw std::runtime_error("wrapped displacement face outside table");const size_t d=di[0]+176*i;
   const auto plan=sourceDisplacementReadPlanCandidate(u32(d+20),vertexCursor,triangleCursor);
   if(plan.nextVertex>dv[1]||plan.nextTriangle>dt[1])throw std::runtime_error("sequential displacement data outside lump");
   const auto corners=sourceDisplacementFaceQuadCandidate(faces[face],vertices,edges,surfedges);std::array<float,3>start{scalar(d),scalar(d+4),scalar(d+8)};
   // The original input initializer ignores elevation; do not silently accept
   // nonfinite format values even though this particular argument is unused.
   if(!std::isfinite(scalar(d+28)))throw std::runtime_error("nonfinite displacement elevation field");
   std::vector<std::array<float,5>>raw(plan.vertexBytes/20);std::memcpy(raw.data(),b.data()+dv[0]+plan.vertexOffset,plan.vertexBytes);std::vector<uint16_t>tags(plan.triangleBytes/2);std::memcpy(tags.data(),b.data()+dt[0]+plan.triangleOffset,plan.triangleBytes);
   const auto geometry=sourceDisplacementCoreGeometryCandidate(u32(d+20),u32(d+32),u32(d+24),corners,start,raw,tags);
   meshes.push_back(sourceDisplacementRuntimeMeshCandidate(geometry.power,geometry.contents,geometry.surfaceFlags,geometry.corners,geometry.positions,geometry.alphas,geometry.triangles));vertexCursor=plan.nextVertex;triangleCursor=plan.nextTriangle;
  }
 }
};
