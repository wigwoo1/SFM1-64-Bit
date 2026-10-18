#include <vector>
#include <memory>
#include <iostream>
#include "studio_material_mixed_queue_owner_candidate.h"
#include "studio_material_mesh_upload_candidate.h"
#include "studio_material_geometry_arena_candidate.h"
#include "studio_material_mesh_capture_candidate.h"
#include "studio_material_vertex_layout_candidate.h"
#include "studio_dynamic_mesh_draw_range_candidate.h"
#include "studio_dynamic_mesh_unlock_candidate.h"
#include "studio_dynamic_mesh_buffer_unlock_candidate.h"
#include "studio_vertex_buffer_unlock_candidate.h"
#include "studio_index_buffer_unlock_candidate.h"
struct Fixture{std::vector<unsigned char> vertices,uploaded;std::vector<uint16_t> indices,uploadedIndices;uint32_t offset=0,byteOffset=0,stride=12;uint64_t format=0;uintptr_t drawReceiver=0;bool position=false,locked=false;unsigned locks=0,unlocks=0,draws=0,bufferUnlocks=0;uint8_t vertexLocked=0,indexLocked=0;StudioDynamicMeshUnlockStateCandidate meshUnlock;StudioVertexBufferUnlockStateCandidate vertexBuffer;StudioIndexBufferUnlockStateCandidate indexBuffer;};
static void upload(uintptr_t receiver,const StudioMaterialMeshUploadSnapshotCandidate& saved){auto& f=*reinterpret_cast<Fixture*>(receiver);
studioMaterialMeshUploadCandidate(saved,f.byteOffset,0, [&](uint32_t vertices,uint32_t indices,auto& descriptor){if(f.locked||vertices*f.stride!=f.vertices.size()||indices!=f.indices.size())throw 1;f.locked=true;++f.locks;descriptor.stride=f.stride;descriptor.vertexOffset=f.offset;descriptor.positionPresent=f.position;descriptor.position=reinterpret_cast<uintptr_t>(f.uploaded.data());descriptor.indexPresent=true;descriptor.indices=reinterpret_cast<uintptr_t>(f.uploadedIndices.data());
f.vertexBuffer={reinterpret_cast<uintptr_t>(f.uploaded.data()),17,f.stride,2,{}};f.indexBuffer={descriptor.indices,0,1,{}};f.vertexLocked=f.indexLocked=1;f.meshUnlock.locked=true;f.meshUnlock.primitive=3;if(f.format){auto layout=studioMaterialVertexLayoutCandidate(descriptor.position,f.format,0);if(layout.stride!=f.stride)throw 4;descriptor.positionPresent=layout.strides[0]!=0;descriptor.position=layout.pointers[0];unsigned fields[]={1,2,3,4,5,14,15,16};for(unsigned i=0;i<8;++i){descriptor.nonposition.present[i]=layout.strides[fields[i]];descriptor.nonposition.pointers[i]=layout.pointers[fields[i]];}}else{descriptor.nonposition.present[0]=1;descriptor.nonposition.pointers[0]=descriptor.position;}},[&](uintptr_t to,uintptr_t from,size_t bytes){if(!f.locked)throw 1;std::memcpy(reinterpret_cast<void*>(to),reinterpret_cast<void*>(from),bytes);},[&](uint32_t vertices,uint32_t indices,const auto& descriptor){
 if(!f.locked)throw 1;
 auto direct=[&](uintptr_t resource){if(!f.locked||!resource)throw 8;++f.bufferUnlocks;return int32_t(0);};
 studioDynamicMeshUnlockCandidate(f.meshUnlock,vertices,indices,reinterpret_cast<uintptr_t>(&descriptor),reinterpret_cast<uintptr_t>(&descriptor.indices),
 [&](uint32_t count,uintptr_t){studioDynamicMeshBufferUnlockCandidate(f.vertexLocked,reinterpret_cast<uintptr_t>(&f.vertexBuffer),count,
 [&](uintptr_t buffer,uint32_t requested){studioVertexBufferUnlockCandidate(*reinterpret_cast<StudioVertexBufferUnlockStateCandidate*>(buffer),requested,[]{return false;},[](auto){throw 8;},direct,[](unsigned){throw 8;});});},
 [&](uint32_t count,uintptr_t){studioDynamicMeshBufferUnlockCandidate(f.indexLocked,reinterpret_cast<uintptr_t>(&f.indexBuffer),count,
 [&](uintptr_t buffer,uint32_t requested){studioIndexBufferUnlockCandidate(*reinterpret_cast<StudioIndexBufferUnlockStateCandidate*>(buffer),requested,[]{return false;},[](auto){throw 8;},direct,[](unsigned){throw 8;});});});
 f.locked=false;++f.unlocks;});}
static void draw(uintptr_t receiver){auto& f=*reinterpret_cast<Fixture*>(receiver);if(f.locked||f.locks!=1||f.unlocks!=1||f.uploaded!=f.vertices||f.byteOffset!=f.offset*f.stride)throw 1;if(f.bufferUnlocks!=2||f.vertexLocked||f.indexLocked||f.meshUnlock.locked||f.vertexBuffer.flags||f.indexBuffer.flags||f.meshUnlock.totalVertices!=f.indices.size()||f.meshUnlock.totalIndices!=f.indices.size()||f.meshUnlock.lastVertices!=uint16_t(f.indices.size())||f.meshUnlock.lastIndices!=f.indices.size()||f.indexBuffer.usedIndices!=f.indices.size()||f.vertexBuffer.usedBytes!=((17+f.stride-1)/f.stride)*f.stride+f.vertices.size())throw 8;for(unsigned i=0;i<f.indices.size();++i)if(f.uploadedIndices[i]!=uint16_t(f.indices[i]+uint16_t(f.offset)))throw 1;++f.draws;}
static void drawRange(uintptr_t receiver,int32_t first,int32_t count){auto& f=*reinterpret_cast<Fixture*>(receiver);auto resolved=studioDynamicMeshDrawRangeCandidate(first,count,5,false,f.meshUnlock.primitive,f.meshUnlock.totalVertices,f.meshUnlock.totalIndices,[]{throw 5;return 0u;});if(resolved.first!=5||resolved.count!=f.indices.size())throw 5;draw(receiver);}
int main(){StudioMaterialMixedQueueOwnerCandidate queue;StudioMaterialGeometryArenaCandidate vertexArena,indexArena;unsigned cases=0;size_t vertexCommitted=0,indexCommitted=0;
for(unsigned count=1;count<=128;++count)for(unsigned position=0;position<2;++position){Fixture f;f.position=position!=0;f.offset=count*977;f.vertices.resize(count*f.stride);f.uploaded.resize(count*f.stride,0xa5);f.indices.resize(count);f.uploadedIndices.resize(count,0xa5a5);for(unsigned i=0;i<f.vertices.size();++i)f.vertices[i]=static_cast<unsigned char>((i*31+count)%251);for(unsigned i=0;i<count;++i)f.indices[i]=uint16_t(i*977+65500);
auto vertexPointer=vertexArena.allocate(f.vertices.size()),indexPointer=indexArena.allocate(f.indices.size()*2);if(!cases){vertexCommitted=vertexArena.committedBytes();indexCommitted=indexArena.committedBytes();if(!vertexCommitted||!indexCommitted||vertexCommitted>0x20000||indexCommitted>0x20000)return 1;}if(vertexPointer!=vertexArena.base()||indexPointer!=indexArena.base()||vertexArena.committedBytes()!=vertexCommitted||indexArena.committedBytes()!=indexCommitted)return 1;
std::memcpy(reinterpret_cast<void*>(vertexPointer),f.vertices.data(),f.vertices.size());std::memcpy(reinterpret_cast<void*>(indexPointer),f.indices.data(),f.indices.size()*2);
StudioMaterialMeshUploadStateCandidate mesh{vertexPointer,indexPointer,count,count,12,7};
studioMaterialMeshUploadSnapshotCandidate(mesh,true,[&](auto saved){queue.enqueueUpload(reinterpret_cast<uintptr_t>(&f),upload,saved);saved={};});queue.enqueueZero(reinterpret_cast<uintptr_t>(&f),draw);
if(mesh.vertexData||mesh.indexData||mesh.primitive!=2||f.locks||f.draws||queue.queuedCount()!=2)return 1;queue.drain();if(f.draws!=1||queue.queuedCount()||f.locked||!vertexArena.usedBytes()||!indexArena.usedBytes())return 1;vertexArena.reset();indexArena.reset();if(vertexArena.usedBytes()||indexArena.usedBytes()||vertexArena.committedBytes()!=vertexCommitted||indexArena.committedBytes()!=indexCommitted)return 1;++cases;
}
uintptr_t receiverBefore[3]={},receiverAfter[3]={};uintptr_t holder=reinterpret_cast<uintptr_t>(receiverBefore);
queue.enqueueDraw(&holder,[](uintptr_t receiver,int32_t first,int32_t count){auto* observed=reinterpret_cast<uintptr_t*>(receiver);observed[0]=1;observed[1]=uint32_t(first);observed[2]=uint32_t(count);},-17,93);
holder=reinterpret_cast<uintptr_t>(receiverAfter);queue.drain();
if(receiverBefore[0]||receiverAfter[0]!=1||receiverAfter[1]!=uint32_t(-17)||receiverAfter[2]!=93)return 15;
std::cout<<"live_draw_receiver=1 ";
unsigned drawCases=0;
for(unsigned bits=0;bits<32;++bits)for(int32_t first:{int32_t(-1),int32_t(0),int32_t(2)})for(int32_t count:{int32_t(0),int32_t(3),int32_t(5)}){
 StudioMaterialMeshUploadStateCandidate mesh{0x1111,0x2222,(bits&1)?3u:0u,(bits&2)?5u:0u,12,7};
 bool overridden=(bits&4)!=0;uint32_t overrideSize=(bits&8)?5u:7u;
 bool empty=!mesh.vertexCount&&!mesh.indexCount;unsigned queries=0,empties=0,uploads=0,draws=0;
 bool expectedReset=first==-1||count==0||(!overridden&&!mesh.indexCount)||uint32_t(first)+uint32_t(count)==(overridden?overrideSize:mesh.indexCount);
 studioMaterialMeshDrawCandidate(mesh,overridden,first,count,[&]{++queries;return overrideSize;},[&]{++empties;},
 [&](auto saved){if(saved.vertexData!=0x1111||saved.indexData!=0x2222||draws)throw 7;++uploads;},
 [&](int32_t savedFirst,int32_t savedCount){if(!uploads||savedFirst!=first||savedCount!=count)throw 7;++draws;});
 if(empties!=unsigned(empty)||uploads!=unsigned(!empty)||draws!=unsigned(!empty)||queries!=unsigned(!empty&&overridden&&first!=-1&&count!=0))return 13;
 if(mesh.vertexData!=((!empty&&expectedReset)?0:0x1111)||mesh.indexData!=((!empty&&expectedReset)?0:0x2222))return 14;
 ++drawCases;
}
std::cout<<"draw_range_cases="<<drawCases<<" ";
unsigned captureCases=0;
for(unsigned bits=0;bits<64;++bits){
 const unsigned vc=(bits&1)?3:0,ic=(bits&2)?5:0;
 const bool vo=(bits&4)!=0,io=(bits&8)!=0;const unsigned mode=(bits&16)?2:0;
 const uint16_t stride=(bits&32)?12:0;
 StudioMaterialMeshUploadStateCandidate mesh{0x1111,0x2222,99,99,stride,mode};
 StudioMaterialMeshCaptureDescriptorCandidate descriptor{9,9,9,9,9,9};unsigned va=0,ia=0,layouts=0;
 studioMaterialMeshCaptureCandidate(mesh,vo,io,vc,ic,0x1234,0x5678,0x3333,descriptor,
 [&](size_t bytes){if(bytes!=vc*stride)throw 3;++va;return uintptr_t(0x4444);},
 [&](size_t bytes){if(bytes!=ic*2)throw 3;++ia;return uintptr_t(0x5555);},
 [&](uintptr_t data,uintptr_t low,uintptr_t high,const auto& d){
 if(data!=(vc?0x4444:0)||low!=(vc?0x1234:0)||high!=(vc?0x5678:0)||d.vertexOffset||d.vertexOffsetBytes)throw 3;++layouts;
 });
 if(mesh.vertexCount!=(vo?0:vc)||mesh.indexCount!=(io?0:ic)||va!=(vc?1u:0u)||ia!=((mode&&ic)?1u:0u)||layouts!=1)return 10;
 if(mesh.vertexData!=(vc?0x4444:0x1111)||mesh.indexData!=((mode&&ic)?0x5555:0x2222)||descriptor.indices!=((mode&&ic)?0x5555:0x3333)||descriptor.indexPresent!=((mode&&ic)?1u:0u)||descriptor.firstIndex||descriptor.indexOffsetBytes)return 11;
 ++captureCases;
}
auto zeroStart=vertexArena.allocate(0);auto zeroNext=vertexArena.allocate(0);if(zeroNext-zeroStart!=16)return 12;vertexArena.reset();
std::cout<<"capture_branch_cases="<<captureCases<<" zero_payload_step=16 ";
unsigned batchCases=0;size_t retainedVertexCommit=0,retainedIndexCommit=0;
for(unsigned batch=0;batch<4;++batch){
 std::vector<std::unique_ptr<Fixture>> fixtures;
 uintptr_t previousVertexEnd=vertexArena.base(),previousIndexEnd=indexArena.base();
 for(unsigned item=1;item<=128;++item){
  auto owned=std::make_unique<Fixture>();auto& f=*owned;f.drawReceiver=reinterpret_cast<uintptr_t>(&f);
  unsigned count=item*8;f.position=(item&1)!=0;f.offset=item*977;
  f.format=f.position?uint64_t(0xff)|(uint64_t(item%8)<<12)|(uint64_t(item%8)<<15):uint64_t(2|4|8|0x10|0x20);
  if(f.position)for(unsigned channel=0;channel<8;++channel)f.format|=uint64_t((item+channel)%5)<<(18+3*channel);
  if(item&2)f.format|=0x400;if(item&4)f.format|=0x800;if(item&8)f.format|=0x200;
  f.stride=studioMaterialVertexLayoutCandidate(0,f.format,0).stride;
  f.vertices.resize(count*f.stride);f.uploaded.resize(count*f.stride,0xa5);
  f.indices.resize(count);f.uploadedIndices.resize(count,0xa5a5);
  for(unsigned i=0;i<f.vertices.size();++i)f.vertices[i]=static_cast<unsigned char>((i*31+item+batch)%251);
  for(unsigned i=0;i<count;++i)f.indices[i]=uint16_t(i*977+65500+batch);
  StudioMaterialMeshUploadStateCandidate mesh{0,0,0,0,uint16_t(f.stride),7};
  StudioMaterialMeshCaptureDescriptorCandidate capture;
  unsigned layoutCalls=0;
  studioMaterialMeshCaptureCandidate(mesh,false,false,count,count,uint32_t(f.format),uint32_t(f.format>>32),0,capture,
   [&](size_t bytes){return vertexArena.allocate(bytes);},[&](size_t bytes){return indexArena.allocate(bytes);},
   [&](uintptr_t data,uintptr_t low,uintptr_t high,auto& descriptor){
    auto layout=studioMaterialVertexLayoutCandidate(data,uint64_t(low)|(uint64_t(high)<<32),0);
    if(layout.stride!=f.stride||layout.pointers[f.position?0:3]!=data||descriptor.vertexOffset||descriptor.vertexOffsetBytes)throw 2;++layoutCalls;
   });
  auto vertices=mesh.vertexData,indices=mesh.indexData;
  if(layoutCalls!=1||capture.indices!=indices||capture.indexPresent!=1||capture.firstIndex||capture.indexOffsetBytes||mesh.vertexCount!=count||mesh.indexCount!=count)return 9;
  if(vertices<previousVertexEnd||indices<previousIndexEnd||(vertices&15)||(indices&15))return 2;
  previousVertexEnd=vertices+f.vertices.size();previousIndexEnd=indices+f.indices.size()*2;
  std::memcpy(reinterpret_cast<void*>(vertices),f.vertices.data(),f.vertices.size());
  std::memcpy(reinterpret_cast<void*>(indices),f.indices.data(),f.indices.size()*2);
  studioMaterialMeshDrawCandidate(mesh,false,-1,int32_t(count),[]{return 0u;},[]{throw 6;},
   [&](auto saved){queue.enqueueUpload(reinterpret_cast<uintptr_t>(&f),upload,saved);},
   [&](int32_t first,int32_t rangeCount){queue.enqueueDraw(&f.drawReceiver,drawRange,first,rangeCount);});
  if(f.locks||f.draws||mesh.vertexData||mesh.indexData)return 3;
  fixtures.push_back(std::move(owned));
 }
 if(queue.queuedCount()!=256||vertexArena.committedBytes()<=0x20000||indexArena.committedBytes()<=0x20000)return 4;
 if(batch&&(vertexArena.committedBytes()!=retainedVertexCommit||indexArena.committedBytes()!=retainedIndexCommit))return 5;
 retainedVertexCommit=vertexArena.committedBytes();retainedIndexCommit=indexArena.committedBytes();
 queue.drain();
 for(const auto& f:fixtures){if(f->draws!=1||f->locked)return 6;++batchCases;}
 if(queue.queuedCount())return 7;
 vertexArena.reset();indexArena.reset();
 if(vertexArena.usedBytes()||indexArena.usedBytes())return 8;
}
std::cout<<"batched_meshes="<<batchCases<<" batches=4 simultaneous_meshes=128 commit_growth_and_reuse=1 captured_before_upload=1 " ;
std::cout<<"deferred_mesh_upload_draw_cases="<<cases<<" position_and_nonposition=1 recovered_unlock_chain=1 arena_owned_upload=1 geometry_arenas_retained_until_drain=1 reused_commit=1\n";}
