#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <new>
#include "studio_material_buffer_release_candidate.h"
// Aux buffers+248/+26c:16MiB reserve/128KiBcommit quantum/initial0/alignment16.
// Serialized producer; reset only after all consumers finish. Positive payloads
// and representable arithmetic required. OOM throws as prototype policy.
class StudioMaterialGeometryArenaCandidate {
 StudioMaterialBufferCandidate buffer;
public:
 StudioMaterialGeometryArenaCandidate(){auto* allocation=VirtualAlloc(nullptr,0x1000000,MEM_RESERVE,PAGE_NOACCESS);if(!allocation)throw std::bad_alloc();auto base=reinterpret_cast<uintptr_t>(allocation);buffer={base,base,base+0x1000000,base,false,0x20000};}
 ~StudioMaterialGeometryArenaCandidate(){studioMaterialBufferReleaseCandidate(buffer,[](uintptr_t address,uintptr_t bytes,int type){return VirtualFree(reinterpret_cast<void*>(address),bytes,type)!=0;});}
 StudioMaterialGeometryArenaCandidate(const StudioMaterialGeometryArenaCandidate&)=delete;
 StudioMaterialGeometryArenaCandidate& operator=(const StudioMaterialGeometryArenaCandidate&)=delete;
 uintptr_t allocate(size_t bytes){auto step=bytes?((bytes+15)&~size_t(15)):size_t(16);auto end=buffer.cursor+step;
 if(end>buffer.committedEnd&&!studioMaterialBufferResizeCandidate(buffer,end,[](uintptr_t address,uintptr_t bytes,int type,int protection){return VirtualAlloc(reinterpret_cast<void*>(address),bytes,type,protection)!=nullptr;},[](uintptr_t address,uintptr_t bytes,int type){return VirtualFree(reinterpret_cast<void*>(address),bytes,type)!=0;},[]{},[](uintptr_t){}))throw std::bad_alloc();
 auto address=buffer.cursor;buffer.cursor=end;return address;}
 void reset(){if(buffer.committedEnd>buffer.base)buffer.cursor=buffer.base;}
 uintptr_t base()const{return buffer.base;}
 size_t usedBytes()const{return size_t(buffer.cursor-buffer.base);}
 size_t committedBytes()const{return size_t(buffer.committedEnd-buffer.base);}
};
