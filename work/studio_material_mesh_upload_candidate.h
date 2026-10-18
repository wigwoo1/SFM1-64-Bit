#pragma once
#include "studio_material_mesh_upload_snapshot_candidate.h"
#include "studio_material_position_vertex_upload_candidate.h"
#include "studio_material_nonposition_vertex_upload_candidate.h"
#include "studio_material_index_upload_candidate.h"
struct StudioMaterialMeshUploadDescriptorCandidate{
 uint32_t stride=0,vertexOffset=0;bool positionPresent=false,indexPresent=false;
 uintptr_t position=0,indices=0;StudioMaterialNonpositionDescriptorCandidate nonposition;
};
// Logical100219e0 adapter; descriptor's native/public x64 ABI remains separate.
// Enabled buffers must be valid; index copy requires positive count.
template<class Lock,class Copy,class Unlock>void studioMaterialMeshUploadCandidate(const StudioMaterialMeshUploadSnapshotCandidate& saved,uint32_t& storedByteOffset,uintptr_t fallbackIndices,Lock lock,Copy copy,Unlock unlock){
 StudioMaterialMeshUploadDescriptorCandidate descriptor;lock(saved.vertexCount,saved.indexCount,descriptor);
 storedByteOffset=descriptor.stride*descriptor.vertexOffset;
 if(descriptor.positionPresent)studioMaterialPositionVertexUploadCandidate(descriptor.position,saved.vertexData,descriptor.stride,saved.vertexBytes,copy);
 else studioMaterialNonpositionVertexUploadCandidate(descriptor.nonposition,saved.vertexData,descriptor.stride,saved.vertexBytes,copy);
 if(saved.indexData&&saved.indexData!=fallbackIndices&&descriptor.indexPresent)studioMaterialIndexUploadCandidate(descriptor.indices,saved.indexData,int(saved.indexCount),descriptor.vertexOffset,copy);
 unlock(saved.vertexCount,saved.indexCount,descriptor);
}
