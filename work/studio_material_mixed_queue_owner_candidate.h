#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cassert>
#include <new>
#include "studio_material_queue_exception_cleanup_candidate.h"
#include "studio_material_zero_argument_call_candidate.h"
#include "studio_material_mesh_upload_call_candidate.h"
#include "studio_material_mesh_draw_candidate.h"
#include "studio_material_buffer_release_candidate.h"
// Logical x64 callback adapter; views consume additional arena bytes versus
// a native virtual functor. Calls are serialized. Failed decal enqueue retains
// the caller reference; successful enqueue transfers it to the queue.
class StudioMaterialMixedQueueOwnerCandidate {
 struct View{void* object;void(*invoke)(void*);void(*release)(void*);};
 struct Node{Node* next;View* call;};
 struct Queue{Node* head=nullptr;Node* tail=nullptr;}queue;
 StudioMaterialBufferCandidate buffer;size_t pending=0;
 void* allocate(size_t bytes,size_t alignment){auto begin=(buffer.cursor+alignment-1)&~uintptr_t(alignment-1);auto end=begin+bytes;
 if(end>buffer.committedEnd&&!studioMaterialBufferResizeCandidate(buffer,end,[](uintptr_t a,uintptr_t n,int t,int p){return VirtualAlloc(reinterpret_cast<void*>(a),n,t,p)!=nullptr;},[](uintptr_t a,uintptr_t n,int t){return VirtualFree(reinterpret_cast<void*>(a),n,t)!=0;},[]{},[](uintptr_t){}))throw std::bad_alloc();buffer.cursor=end;return reinterpret_cast<void*>(begin);}
 void append(View view){const auto before=buffer.cursor;try{auto* saved=new(allocate(sizeof(View),alignof(View)))View(view);auto* node=new(allocate(sizeof(Node),alignof(Node)))Node{nullptr,saved};studioMaterialQueueAppendCandidate(queue,node);++pending;}catch(...){buffer.cursor=before;throw;}}
public:
 StudioMaterialMixedQueueOwnerCandidate(){auto* allocation=VirtualAlloc(nullptr,0x800000,MEM_RESERVE,PAGE_NOACCESS);if(!allocation)throw std::bad_alloc();auto base=reinterpret_cast<uintptr_t>(allocation);buffer={base,base,base+0x800000,base,false,0x10000};if(!VirtualAlloc(allocation,0x40000,MEM_COMMIT,PAGE_READWRITE)){VirtualFree(allocation,0,MEM_RELEASE);throw std::bad_alloc();}buffer.committedEnd+=0x40000;buffer.dirty=true;}
 ~StudioMaterialMixedQueueOwnerCandidate(){assert(!queue.head);studioMaterialBufferReleaseCandidate(buffer,[](uintptr_t a,uintptr_t n,int t){return VirtualFree(reinterpret_cast<void*>(a),n,t)!=0;});}
 StudioMaterialMixedQueueOwnerCandidate(const StudioMaterialMixedQueueOwnerCandidate&)=delete;
 StudioMaterialMixedQueueOwnerCandidate& operator=(const StudioMaterialMixedQueueOwnerCandidate&)=delete;
 void enqueue(StudioDecalQueuedCallCandidate* call){call->addReference();try{append({call,[](void* object){static_cast<StudioDecalQueuedCallCandidate*>(object)->invoke();},[](void* object){static_cast<StudioDecalQueuedCallCandidate*>(object)->releaseReference([](auto* p){delete p;});}});}catch(...){call->releaseReference([](auto* p){delete p;});throw;}call->releaseReference([](auto* p){delete p;});}
 void enqueueZero(uintptr_t owner,StudioMaterialZeroArgumentCallCandidate::Method method){auto* call=new(allocate(sizeof(StudioMaterialZeroArgumentCallCandidate),alignof(StudioMaterialZeroArgumentCallCandidate)))StudioMaterialZeroArgumentCallCandidate;call->owner=owner;call->method=method;
 append({call,[](void* object){static_cast<StudioMaterialZeroArgumentCallCandidate*>(object)->invoke();},[](void* object){static_cast<StudioMaterialZeroArgumentCallCandidate*>(object)->releaseReference([](auto*,unsigned flags){assert(flags==0);});}});}
 void enqueueUpload(uintptr_t owner,StudioMaterialMeshUploadCallCandidate::Method method,const StudioMaterialMeshUploadSnapshotCandidate& saved){auto* call=new(allocate(sizeof(StudioMaterialMeshUploadCallCandidate),alignof(StudioMaterialMeshUploadCallCandidate)))StudioMaterialMeshUploadCallCandidate;call->owner=owner;call->method=method;call->saved=saved;
 append({call,[](void* object){static_cast<StudioMaterialMeshUploadCallCandidate*>(object)->invoke();},[](void* object){static_cast<StudioMaterialMeshUploadCallCandidate*>(object)->releaseReference([](auto*,unsigned flags){assert(flags==0);});}});}
 void enqueueDraw(uintptr_t* receiverHolder,StudioMaterialMeshDrawCallCandidate::Method method,int32_t first,int32_t count){auto* call=new(allocate(sizeof(StudioMaterialMeshDrawCallCandidate),alignof(StudioMaterialMeshDrawCallCandidate)))StudioMaterialMeshDrawCallCandidate;call->receiverHolder=receiverHolder;call->method=method;call->first=first;call->count=count;
 append({call,[](void* object){static_cast<StudioMaterialMeshDrawCallCandidate*>(object)->invoke();},[](void* object){static_cast<StudioMaterialMeshDrawCallCandidate*>(object)->releaseReference([](auto*,unsigned flags){assert(flags==0);});}});}
 void drain(){studioMaterialQueueDrainWithExceptionCleanupCandidate(queue,[](View* view){view->invoke(view->object);},[&](View* view){view->release(view->object);--pending;},[&](bool decommit){assert(!decommit);buffer.cursor=buffer.base;});}
 size_t queuedCount()const{return pending;}
 uintptr_t arenaBase()const{return buffer.base;}
};


