#pragma once
#include "studio_material_mesh_upload_snapshot_candidate.h"
// Native serialized arena functor; snapshot pointers do not own source geometry.
struct StudioMaterialMeshUploadCallCandidate{
 using Method=void(*)(uintptr_t,const StudioMaterialMeshUploadSnapshotCandidate&);
 uint32_t references=1;Method method=nullptr;uintptr_t owner=0;intptr_t adjustment=0;
 StudioMaterialMeshUploadSnapshotCandidate saved{};
 uint32_t addReference(){return ++references;}
 template<class Destroy>uint32_t releaseReference(Destroy destroy){auto remaining=--references;if(!remaining)destroy(this,0u);return remaining;}
 void invoke(){method(owner+uintptr_t(adjustment),saved);}
};
