#pragma once
#include <cstdint>
struct StudioDynamicMeshUnlockStateCandidate{
 uint32_t totalVertices=0,totalIndices=0,lastIndices=0;uint16_t lastVertices=0;
 bool locked=false;uint32_t primitive=0;
};
// Logical100070e0; index-subdescriptor mapping belongs to the ABI adapter.
// Native counts are32-bit and last vertex count truncates to16-bit.
template<class VertexUnlock,class IndexUnlock>
void studioDynamicMeshUnlockCandidate(StudioDynamicMeshUnlockStateCandidate& state,uint32_t vertices,uint32_t indices,uintptr_t descriptor,uintptr_t indexDescriptor,VertexUnlock vertexUnlock,IndexUnlock indexUnlock){
 state.totalVertices+=vertices;state.totalIndices+=indices;
 vertexUnlock(vertices,descriptor);
 if(state.primitive)indexUnlock(indices,descriptor?indexDescriptor:0);
 state.lastIndices=indices;state.lastVertices=uint16_t(vertices);state.locked=false;
}
