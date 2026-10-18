#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <new>
#include <cassert>
#include "studio_material_buffer_resize_candidate.h"
#include "studio_material_buffer_release_candidate.h"
#include "studio_material_queue_exception_cleanup_candidate.h"
#include "studio_material_external_enqueue_candidate.h"
// Native context reserves 8 MiB, initially commits 256 KiB, and grows
// on 64 KiB boundaries. x64 nodes require pointer alignment. Calls are serialized and the queue must be drained
// before destruction. Reserved node storage survives functor deletion.
class StudioMaterialQueueOwnerCandidate {
 void* allocation=nullptr;StudioMaterialBufferCandidate buffer;
 StudioMaterialQueueCandidate queue;size_t queued=0;
public:
 explicit StudioMaterialQueueOwnerCandidate(size_t reserveBytes=0x800000,size_t initialCommit=0x40000,size_t commitAlignment=0x10000){
  SYSTEM_INFO info;GetSystemInfo(&info);
  if(!commitAlignment)commitAlignment=info.dwPageSize;
  commitAlignment=(commitAlignment+info.dwPageSize-1)&~size_t(info.dwPageSize-1);
  if(commitAlignment&(commitAlignment-1))throw std::bad_alloc();
  reserveBytes=(reserveBytes+commitAlignment-1)&~(commitAlignment-1);
  initialCommit=(initialCommit+commitAlignment-1)&~(commitAlignment-1);
  if(!reserveBytes||initialCommit>reserveBytes)throw std::bad_alloc();
  allocation=VirtualAlloc(nullptr,reserveBytes,MEM_RESERVE,PAGE_NOACCESS);if(!allocation)throw std::bad_alloc();
  auto base=reinterpret_cast<uintptr_t>(allocation);buffer={base,base,base+reserveBytes,base,false,commitAlignment};
  if(initialCommit){if(!VirtualAlloc(allocation,initialCommit,MEM_COMMIT,PAGE_READWRITE)){VirtualFree(allocation,0,MEM_RELEASE);allocation=nullptr;throw std::bad_alloc();}buffer.committedEnd+=initialCommit;buffer.dirty=true;}
 }

 ~StudioMaterialQueueOwnerCandidate(){assert(!queue.head);studioMaterialBufferReleaseCandidate(buffer,[](uintptr_t address,uintptr_t bytes,int type){return VirtualFree(reinterpret_cast<void*>(address),bytes,type)!=0;});}
 StudioMaterialQueueOwnerCandidate(const StudioMaterialQueueOwnerCandidate&)=delete;
 StudioMaterialQueueOwnerCandidate& operator=(const StudioMaterialQueueOwnerCandidate&)=delete;
 void enqueue(StudioDecalQueuedCallCandidate* call){
  studioMaterialExternalEnqueueCandidate(call,[&](auto* supplied){uintptr_t size=(sizeof(StudioMaterialQueueNodeCandidate)+alignof(StudioMaterialQueueNodeCandidate)-1)&~uintptr_t(alignof(StudioMaterialQueueNodeCandidate)-1);uintptr_t end=buffer.cursor+size;if(end>buffer.committedEnd){bool ok=studioMaterialBufferResizeCandidate(buffer,end,[](uintptr_t address,uintptr_t bytes,int type,int protection){return VirtualAlloc(reinterpret_cast<void*>(address),bytes,type,protection)!=nullptr;},[](uintptr_t address,uintptr_t bytes,int type){return VirtualFree(reinterpret_cast<void*>(address),bytes,type)!=0;},[]{},[](uintptr_t){});if(!ok)throw std::bad_alloc();}auto* node=new(reinterpret_cast<void*>(buffer.cursor)) StudioMaterialQueueNodeCandidate;buffer.cursor=end;node->call=supplied;studioMaterialQueueAppendCandidate(queue,node);++queued;},[](auto* object){delete object;});
 }
 void drain(){studioMaterialQueueDrainWithExceptionCleanupCandidate(queue,[](auto* call){call->invoke();},[&](auto* call){call->releaseReference([](auto* object){delete object;});--queued;},[&](bool decommit){assert(!decommit);if(buffer.base&&buffer.committedEnd>buffer.base)buffer.cursor=buffer.base;});}
 size_t queuedCount()const{return queued;}
 size_t reservedBytes()const{return size_t(buffer.reservedEnd-buffer.base);}
 size_t committedBytes()const{return size_t(buffer.committedEnd-buffer.base);}
 const StudioMaterialQueueCandidate& queueState()const{return queue;}
};

