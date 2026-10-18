#pragma once
#include "studio_decal_resource_create_candidate.h"
#include "studio_decal_entry_delete_candidate.h"
template<class Count,class Lock,class Unlock,class Grow,class Warn,class Frontier,class Entries>
inline StudioDecalPointerNodeCandidate* studioDecalTypedResourceCreateLiveCandidate(StudioDecalPointerPoolCandidate& pool,uintptr_t descriptor,Count readCount,Lock lock,Unlock unlock,Grow grow,Warn warn,Frontier beyond,Entries allocateEntries){
 return studioDecalResourceCreateWithEntriesLiveCandidate(pool,descriptor,readCount,lock,unlock,grow,warn,beyond,allocateEntries,[](StudioDecalEntryCandidate* entries,int32_t i){auto& entry=entries[i];entry.buffer={};entry.list={};entry.list.pool.maximumIndex=0xffff;},[](StudioDecalEntryCandidate* entries,int32_t i){entries[i].chainHead=0xffff;});
}
