#include "studio_decal_pointer_allocate_candidate.h"
#include "studio_decal_pointer_insert_candidate.h"
#include "studio_decal_pointer_frontier_candidate.h"
#include <memory>
#include <vector>
#include <cstdio>
#include <cstdlib>
void check(bool x){if(!x)std::abort();}
int main(){StudioDecalPointerPoolCandidate pool;std::vector<std::unique_ptr<StudioDecalPointerBlockCandidate>> blocks;std::vector<std::unique_ptr<StudioDecalPointerNodeCandidate[]>> buffers;std::vector<StudioDecalPointerNodeCandidate*> allocated;unsigned growths=0;
 auto grow=[&](auto& state,int additional){studioDecalPointerGrowCandidate(state,additional,[&](int count){check(state.capacity>=count);auto block=std::make_unique<StudioDecalPointerBlockCandidate>();auto nodes=std::make_unique<StudioDecalPointerNodeCandidate[]>(count);block->nodes=nodes.get();auto* result=block.get();blocks.push_back(std::move(block));buffers.push_back(std::move(nodes));++growths;return result;});};
 for(unsigned i=0;i<4096;++i){auto* node=studioDecalPointerAllocateCandidate(pool,0,grow,[]{std::abort();});check(node&&node->previous==node&&node->next==node);studioDecalPointerInsertCandidate(pool,nullptr,node,[](auto& state,auto* node){return studioDecalPointerBeyondFrontierCandidate(state,node,[](uintptr_t address,uintptr_t frontier){return address>frontier;});});node->descriptor=uintptr_t(0x1234567800000000ull)+i;allocated.push_back(node);}
 check(pool.capacity==4096&&growths==12&&pool.freshCount==4096&&pool.linkedCount==4096);auto* current=pool.head;for(unsigned i=0;i<4096;++i){check(current==allocated[i]&&current->previous==(i?allocated[i-1]:nullptr));current=current->next;}check(!current&&pool.tail==allocated.back());
 for(unsigned i=0;i<4096;++i){check(allocated[i]->descriptor==uintptr_t(0x1234567800000000ull)+i);studioDecalPointerUnlinkCandidate(pool,allocated[i],[](auto& state,auto* node){return studioDecalPointerBeyondFrontierCandidate(state,node,[](uintptr_t address,uintptr_t frontier){return address>frontier;});});allocated[i]->next=pool.freeHead;pool.freeHead=allocated[i];}
 for(unsigned i=0;i<4096;++i){auto* node=studioDecalPointerAllocateCandidate(pool,0x101,grow,[]{std::abort();});check(node==allocated[4095-i]&&!node->previous&&!node->next);check(node->descriptor==uintptr_t(0x1234567800000000ull)+4095-i);}check(!pool.freeHead&&pool.freshCount==4096&&growths==12&&pool.linkedCount==0&&!pool.head&&!pool.tail);
 StudioDecalPointerPoolCandidate failure;failure.field28=77;unsigned warnings=0;check(!studioDecalPointerAllocateCandidate(failure,0,[](auto&,int){},[&]{++warnings;}));check(warnings==1&&failure.field28==0);
 std::puts("PASS:4096 stable full64-bit decal pointers/12 growths/4096 free reuses, untouched payloads and failed growth; actual insertion/unlink integrated; owned frontier classification with explicit unsigned x64 ordering integrated; native allocator parity pending.");
}
