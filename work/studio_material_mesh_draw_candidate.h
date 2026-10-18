#pragma once
#include "studio_material_mesh_upload_snapshot_candidate.h"
// Logical10023cd0 orchestration; range addition preserves32-bit wrap.
template<class OverrideCount,class Empty,class Upload,class Draw>
void studioMaterialMeshDrawCandidate(StudioMaterialMeshUploadStateCandidate& mesh,bool indexOverride,int32_t first,int32_t count,OverrideCount overrideCount,Empty empty,Upload upload,Draw draw){
 if(!mesh.vertexCount&&!mesh.indexCount){empty();return;}
 bool reset=true;
 if(first!=-1&&count!=0){uint32_t end=uint32_t(first)+uint32_t(count);if(indexOverride)reset=end==overrideCount();else if(mesh.indexCount)reset=end==mesh.indexCount;}
 studioMaterialMeshUploadSnapshotCandidate(mesh,reset,upload);draw(first,count);
}
struct StudioMaterialMeshDrawCallCandidate {
 using Method=void(*)(uintptr_t,int32_t,int32_t);
 uint32_t references=1;Method method=nullptr;uintptr_t* receiverHolder=nullptr;intptr_t adjustment=0;int32_t first=0,count=0;
 template<class Destroy>uint32_t releaseReference(Destroy destroy){auto remaining=--references;if(!remaining)destroy(this,0u);return remaining;}
 void invoke(){method(*receiverHolder+uintptr_t(adjustment),first,count);}
};
