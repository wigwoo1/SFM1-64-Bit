#pragma once
#include "studio_decal_pointer_insert_candidate.h"
#include <cstring>
#include <initializer_list>
// Original32-byte entry schema only; pointer-bearing public type unresolved.
inline void studioDecalEntryConstructorCandidate(unsigned char* entry){
 uint32_t zero=0;uint16_t invalid=0xffff,wordZero=0;
 for(unsigned offset:{4u,8u,12u,28u})std::memcpy(entry+offset,&zero,4);
 for(unsigned offset:{16u,18u,20u,26u})std::memcpy(entry+offset,&invalid,2);
 for(unsigned offset:{22u,24u})std::memcpy(entry+offset,&wordZero,2);
}
inline void studioDecalEntryInitializeCandidate(unsigned char* entry){studioDecalEntryConstructorCandidate(entry);uint16_t invalid=0xffff;std::memcpy(entry,&invalid,2);}
//10008A20 logical creation. Services must supply non-null node/entry storage
// for positive counts, matching observed unsafe original failure paths.
template<class Count,class Lock,class Unlock,class Grow,class Warn,class Frontier,class Entries,class Initialize,class Head>
inline StudioDecalPointerNodeCandidate* studioDecalResourceCreateWithEntriesLiveCandidate(StudioDecalPointerPoolCandidate& pool,uintptr_t descriptor,Count readCount,Lock lock,Unlock unlock,Grow grow,Warn warn,Frontier beyond,Entries allocateEntries,Initialize initializeEntry,Head initializeHead){
 if(!descriptor||readCount()<=0)return nullptr;
 lock();auto* node=studioDecalPointerAllocateCandidate(pool,0,grow,warn);
 if(node)studioDecalPointerInsertCandidate(pool,nullptr,node,beyond);
 unlock();node->descriptor=descriptor;
 const int32_t count=readCount();auto* entries=allocateEntries(count);for(int32_t i=0;i<count;++i)initializeEntry(entries,i);
 node->entries=reinterpret_cast<uintptr_t>(entries);node->entryCount=readCount();
 if(readCount()>0){int32_t i=0;do{initializeHead(entries,i);++i;}while(i<readCount());}return node;
}
template<class Count,class Lock,class Unlock,class Grow,class Warn,class Frontier,class Entries>
inline StudioDecalPointerNodeCandidate* studioDecalResourceCreateLiveCandidate(StudioDecalPointerPoolCandidate& pool,uintptr_t descriptor,Count readCount,Lock lock,Unlock unlock,Grow grow,Warn warn,Frontier beyond,Entries allocateEntries){
 return studioDecalResourceCreateWithEntriesLiveCandidate(pool,descriptor,readCount,lock,unlock,grow,warn,beyond,allocateEntries,[](auto* entries,int32_t i){studioDecalEntryConstructorCandidate(entries+size_t(i)*32);},[](auto* entries,int32_t i){uint16_t invalid=0xffff;std::memcpy(entries+size_t(i)*32,&invalid,2);});
}

template<class Lock,class Unlock,class Grow,class Warn,class Frontier,class Entries>
inline StudioDecalPointerNodeCandidate* studioDecalResourceCreateCandidate(StudioDecalPointerPoolCandidate& pool,uintptr_t descriptor,int32_t count,Lock lock,Unlock unlock,Grow grow,Warn warn,Frontier beyond,Entries allocateEntries){return studioDecalResourceCreateLiveCandidate(pool,descriptor,[&]{return count;},lock,unlock,grow,warn,beyond,allocateEntries);}
