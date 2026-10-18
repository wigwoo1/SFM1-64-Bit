#pragma once
#include <cstdint>
struct StudioDecalPointerNodeCandidate{uintptr_t descriptor=0,entries=0;int32_t entryCount=0;StudioDecalPointerNodeCandidate* previous=nullptr;StudioDecalPointerNodeCandidate* next=nullptr;};
struct StudioDecalPointerBlockCandidate{StudioDecalPointerBlockCandidate* next=nullptr;int32_t count=0;StudioDecalPointerNodeCandidate* nodes=nullptr;};
struct StudioDecalPointerFrontierCandidate{StudioDecalPointerBlockCandidate* block=nullptr;int32_t index=-1;};
inline bool studioDecalFrontierValidCandidate(StudioDecalPointerFrontierCandidate f){return f.block&&f.index>=0&&f.index<f.block->count;}
inline StudioDecalPointerFrontierCandidate studioDecalFrontierAdvanceCandidate(StudioDecalPointerFrontierCandidate f){if(!studioDecalFrontierValidCandidate(f))return {};if(f.index+1<f.block->count)return {f.block,f.index+1};return f.block->next?StudioDecalPointerFrontierCandidate{f.block->next,0}:StudioDecalPointerFrontierCandidate{};}
struct StudioDecalPointerPoolCandidate{StudioDecalPointerBlockCandidate* first=nullptr;StudioDecalPointerNodeCandidate* freeHead=nullptr;StudioDecalPointerFrontierCandidate frontier;uint32_t freshCount=0,field28=0;int32_t capacity=0,growth=0;StudioDecalPointerNodeCandidate* head=nullptr;StudioDecalPointerNodeCandidate* tail=nullptr;uint32_t linkedCount=0;};
//10006E80 logical x64 allocator; original20-byte object layout is not reused.
// Stable blocks/nodes, representable capacity arithmetic are preconditions.
template<class Grow,class Warn>
inline StudioDecalPointerNodeCandidate* studioDecalPointerAllocateCandidate(StudioDecalPointerPoolCandidate& pool,uint32_t initialize,Grow grow,Warn warn){
 auto* node=pool.freeHead;if(node)pool.freeHead=node->next;
 else{auto select=[&](){return studioDecalFrontierValidCandidate(pool.frontier)?studioDecalFrontierAdvanceCandidate(pool.frontier):StudioDecalPointerFrontierCandidate{pool.first,pool.first?0:-1};};auto candidate=select();if(!studioDecalFrontierValidCandidate(candidate)){grow(pool,1);pool.field28=0;candidate=select();if(!studioDecalFrontierValidCandidate(candidate)){warn();return nullptr;}}node=candidate.block->nodes+candidate.index;pool.frontier=candidate;++pool.freshCount;}
 node->previous=node->next=uint8_t(initialize)?nullptr:node;return node;
}
//100037B0 bounded positive arithmetic; capacity committed before block service.
template<class Allocate>
inline void studioDecalPointerGrowCandidate(StudioDecalPointerPoolCandidate& pool,int32_t additional,Allocate allocate){
 if(additional<=0)return;int32_t count=pool.growth?pool.growth:pool.capacity?pool.capacity:2;if(count<additional)count=((count+additional-1)/count)*count;pool.capacity+=count;auto* block=allocate(count);block->next=nullptr;block->count=count;if(!pool.first)pool.first=block;else{auto* last=pool.first;while(last->next)last=last->next;last->next=block;}
}
