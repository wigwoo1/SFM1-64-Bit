#pragma once
#include "studio_material_mesh_upload_snapshot_candidate.h"
// Logical adapter for 10022500. Layout construction remains a shader-service
// callback; this does not define the native descriptor ABI. Serialized capture,
// nonnegative bounded counts and successful allocations are required.
struct StudioMaterialMeshCaptureDescriptorCandidate {
 uintptr_t indices=0;uint32_t vertexOffset=0,vertexOffsetBytes=0;
 uint32_t firstIndex=0,indexOffsetBytes=0,indexPresent=0;
};
template<class VertexAllocate,class IndexAllocate,class Layout>
void studioMaterialMeshCaptureCandidate(StudioMaterialMeshUploadStateCandidate& mesh,
 bool vertexOverride,bool indexOverride,uint32_t requestedVertices,uint32_t requestedIndices,
 uintptr_t formatLow,uintptr_t formatHigh,uintptr_t fallbackIndices,
 StudioMaterialMeshCaptureDescriptorCandidate& descriptor,
 VertexAllocate allocateVertices,IndexAllocate allocateIndices,Layout layout){
 mesh.vertexCount=vertexOverride?0:requestedVertices;
 mesh.indexCount=indexOverride?0:requestedIndices;
 descriptor.vertexOffset=descriptor.vertexOffsetBytes=0;
 if(requestedVertices){
  mesh.vertexData=allocateVertices(uint32_t(mesh.stride)*requestedVertices);
  layout(mesh.vertexData,formatLow,formatHigh,descriptor);
 }else layout(0,0,0,descriptor);
 if(mesh.primitive&&requestedIndices){
  mesh.indexData=allocateIndices(requestedIndices*uint32_t(2));
  descriptor.indices=mesh.indexData;descriptor.indexPresent=1;
 }else{descriptor.indices=fallbackIndices;descriptor.indexPresent=0;}
 descriptor.firstIndex=descriptor.indexOffsetBytes=0;
}
