#include <vector>
#include <iostream>
#include "studio_material_mesh_upload_snapshot_candidate.h"
int main(){unsigned cases=0;for(unsigned reset=0;reset<2;++reset){std::vector<StudioMaterialMeshUploadSnapshotCandidate> queued;
for(unsigned i=0;i<4096;++i){StudioMaterialMeshUploadStateCandidate mesh{0x100000001ULL+i*16,0x200000001ULL+i*8,i*65537u,i+1,uint16_t(i*17),7};auto before=mesh;studioMaterialMeshUploadSnapshotCandidate(mesh,reset!=0,[&](auto saved){if(mesh.vertexData!=before.vertexData||mesh.indexData!=before.indexData)throw 1;queued.push_back(saved);});
if(mesh.vertexCount!=before.vertexCount||mesh.indexCount!=before.indexCount||mesh.stride!=before.stride||mesh.vertexData!=(reset?0:before.vertexData)||mesh.indexData!=(reset?0:before.indexData)||mesh.primitive!=(reset?2u:7u))return 1;mesh={};++cases;}
for(unsigned i=0;i<4096;++i){auto& saved=queued[i];if(saved.vertexData!=0x100000001ULL+i*16||saved.indexData!=0x200000001ULL+i*8||saved.vertexCount!=i*65537u||saved.indexCount!=i+1||saved.vertexBytes!=uint32_t(uint16_t(i*17))*uint32_t(i*65537u))return 1;}
}std::cout<<"upload_snapshot_cases="<<cases<<" fullwidth_pointers=1 copied_before_reset=1 retained_counts=1\n";}
