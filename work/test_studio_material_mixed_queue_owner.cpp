#include <vector>
#include <iostream>
#include "studio_material_mixed_queue_owner_candidate.h"
#include "studio_material_context_nesting_candidate.h"
#include "studio_decal_resource_owner_candidate.h"
static std::vector<unsigned> events;
static StudioMaterialMixedQueueOwnerCandidate* appendQueue;
static void appended(uintptr_t){events.push_back(4);}
static void begin(uintptr_t){events.push_back(1);}static void end(uintptr_t){events.push_back(3);if(appendQueue)appendQueue->enqueueZero(0x100000001ULL,appended);}
static void cleanup(uintptr_t owner,uintptr_t argument){events.push_back(2);reinterpret_cast<StudioDecalResourceOwnerCandidate*>(owner)->release(reinterpret_cast<StudioDecalPointerNodeCandidate*>(argument),[](auto*){},[](uint16_t){throw 1;},[](uintptr_t){throw 1;});}
int main(){StudioMaterialMixedQueueOwnerCandidate queue;StudioDecalResourceOwnerCandidate owner;auto base=queue.arenaBase();
for(unsigned depth=1;depth<=128;++depth){events.clear();appendQueue=depth%2?&queue:nullptr;uint32_t nesting=0;for(unsigned i=0;i<depth;++i)studioMaterialBeginCandidate(nesting,[&]{queue.enqueueZero(0x100000001ULL,begin);});
int count=int(depth);auto* resource=owner.create(0x100000001ULL,[&]{return count;});uintptr_t argument=reinterpret_cast<uintptr_t>(resource);studioDecalQueueCallCandidate(reinterpret_cast<uintptr_t>(&owner),cleanup,&argument,[]{return new StudioDecalQueuedCallCandidate;},[&](auto* call){queue.enqueue(call);if(call->references.load()!=1)throw 1;});argument=0;
for(unsigned i=0;i<depth;++i)studioMaterialEndCandidate(nesting,[&]{queue.enqueueZero(0x100000001ULL,end);});
if(nesting||queue.queuedCount()!=3||owner.activeArrayCount()!=1||!events.empty())return 1;queue.drain();if(events!=(depth%2?std::vector<unsigned>{1,2,3,4}:std::vector<unsigned>{1,2,3})||queue.queuedCount()||owner.activeArrayCount()||owner.poolState().linkedCount||queue.arenaBase()!=base)return 1;queue.drain();
}std::cout<<"mixed_queue_lifetimes=128 actual_resource_releases=128 arena_functors=320 callback_appends=64 stable_arena=1\n";}
