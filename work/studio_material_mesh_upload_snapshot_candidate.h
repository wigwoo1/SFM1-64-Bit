#pragma once
#include <cstdint>
struct StudioMaterialMeshUploadStateCandidate{uintptr_t vertexData=0,indexData=0;uint32_t vertexCount=0,indexCount=0;uint16_t stride=0;uint32_t primitive=2;};
struct StudioMaterialMeshUploadSnapshotCandidate{uintptr_t vertexData;uint32_t vertexCount,vertexBytes;uintptr_t indexData;uint32_t indexCount;};
template<class Enqueue>void studioMaterialMeshUploadSnapshotCandidate(StudioMaterialMeshUploadStateCandidate& mesh,bool reset,Enqueue enqueue){
 StudioMaterialMeshUploadSnapshotCandidate saved{mesh.vertexData,mesh.vertexCount,uint32_t(mesh.stride)*mesh.vertexCount,mesh.indexData,mesh.indexCount};enqueue(saved);
 if(reset){mesh.vertexData=mesh.indexData=0;mesh.primitive=2;}
}
