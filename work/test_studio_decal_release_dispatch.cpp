#include <vector>
#include <iostream>
#include <stdexcept>
#include "studio_decal_release_dispatch_candidate.h"
#include "studio_decal_queued_call_candidate.h"
#include "studio_decal_resource_owner_candidate.h"
#include "studio_material_queue_owner_candidate.h"
static std::vector<unsigned> events;
static void check(bool v){if(!v)throw std::runtime_error("release dispatch contract");}
static void cleanup(uintptr_t owner,uintptr_t argument){events.push_back(6);reinterpret_cast<StudioDecalResourceOwnerCandidate*>(owner)->release(reinterpret_cast<StudioDecalPointerNodeCandidate*>(argument),[](auto*){},[](uint16_t){throw std::runtime_error("unexpected chain");},[](uintptr_t){throw std::runtime_error("unexpected entry buffer");});}
int main(){try{unsigned cases=0,queued=0,direct=0;
for(unsigned count=1;count<=128;++count)for(unsigned hasQueue=0;hasQueue<2;++hasQueue)for(unsigned enabled=0;enabled<2;++enabled){StudioDecalResourceOwnerCandidate owner;int entries=int(count);auto* node=owner.create(uintptr_t(0x100000000ULL+count),[&]{return entries;});StudioMaterialQueueOwnerCandidate materialQueue;bool enqueued=false;events.clear();
studioDecalReleaseDispatchCandidate(reinterpret_cast<uintptr_t>(node),[&]{events.push_back(1);return uintptr_t(0x200000001ULL);},[&](uintptr_t context){check(context==0x200000001ULL);events.push_back(2);},[&](uintptr_t context){check(context==0x200000001ULL);events.push_back(3);return hasQueue?reinterpret_cast<uintptr_t>(&materialQueue):uintptr_t(0);},[&]{events.push_back(4);return enabled!=0;},[&]{events.push_back(5);return reinterpret_cast<uintptr_t>(&owner);},[&](uintptr_t queue,uintptr_t receiver,const uintptr_t* argument){check(queue==reinterpret_cast<uintptr_t>(&materialQueue));events.push_back(7);studioDecalQueueCallCandidate(receiver,&cleanup,argument,[]{return new StudioDecalQueuedCallCandidate;},[&](auto* call){materialQueue.enqueue(call);check(call->references.load()==1);enqueued=true;});},&cleanup,[&](uintptr_t context){check(context==0x200000001ULL);events.push_back(8);},[&](uintptr_t context){check(context==0x200000001ULL);events.push_back(9);});
std::vector<unsigned> expected{1,2,3};if(hasQueue)expected.push_back(4);expected.push_back(5);expected.push_back(hasQueue&&enabled?7:6);expected.push_back(8);expected.push_back(9);check(events==expected);
if(hasQueue&&enabled){check(enqueued&&materialQueue.queuedCount()==1&&owner.activeArrayCount()==1&&node->entries);materialQueue.drain();check(events.back()==6&&materialQueue.queuedCount()==0&&!materialQueue.queueState().head&&!materialQueue.queueState().tail);materialQueue.drain();check(materialQueue.committedBytes()==0x40000);++queued;}else{check(!enqueued&&materialQueue.queuedCount()==0);++direct;}check(owner.activeArrayCount()==0&&!node->entries&&!owner.poolState().linkedCount&&owner.poolState().freeHead==node);++cases;
}
std::cout<<"memory_backed_dispatch_cases="<<cases<<" queued="<<queued<<" direct="<<direct<<"\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
