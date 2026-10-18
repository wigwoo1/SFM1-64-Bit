#include <vector>
#include <iostream>
#include <stdexcept>
#include "studio_decal_queued_call_candidate.h"
#include "studio_decal_resource_owner_candidate.h"
#include "studio_material_external_enqueue_candidate.h"
#include "studio_material_queue_drain_candidate.h"
static unsigned releases;
static void check(bool v){if(!v)throw std::runtime_error("queued owner contract");}
static void releaseResource(uintptr_t receiver,uintptr_t argument){auto* owner=reinterpret_cast<StudioDecalResourceOwnerCandidate*>(receiver);auto* node=reinterpret_cast<StudioDecalPointerNodeCandidate*>(argument);check(node->entries!=0);owner->release(node,[](auto* n){check(n->entries!=0);},[](uint16_t){throw std::runtime_error("unexpected chain");},[](uintptr_t){throw std::runtime_error("unexpected entry buffer");});check(!node->entries);++releases;}
int main(){try{StudioDecalResourceOwnerCandidate owner;unsigned jobFrees=0;size_t firstBlocks=0;
for(unsigned round=0;round<2;++round){std::vector<StudioDecalQueuedCallCandidate*> queue;StudioMaterialQueueCandidate linkedQueue;std::vector<StudioMaterialQueueNodeCandidate> queueNodes(1024);std::vector<StudioDecalPointerNodeCandidate*> nodes;
for(unsigned i=0;i<1024;++i){int count=1+int(i%11);auto* node=owner.create(uintptr_t(0x100000000ULL+i),[&]{return count;});nodes.push_back(node);uintptr_t localArgument=reinterpret_cast<uintptr_t>(node);studioDecalQueueCallCandidate(reinterpret_cast<uintptr_t>(&owner),&releaseResource,&localArgument,[]{return new StudioDecalQueuedCallCandidate;},[&](auto* call){studioMaterialExternalEnqueueCandidate(call,[&](auto* supplied){check(supplied==call&&call->references.load()==2);queue.push_back(supplied);queueNodes[i].call=supplied;studioMaterialQueueAppendCandidate(linkedQueue,&queueNodes[i]);},[](auto*){throw std::runtime_error("premature enqueue free");});check(call->references.load()==1);});localArgument=0;check(queue.back()->argument==reinterpret_cast<uintptr_t>(node));}
check(owner.activeArrayCount()==1024&&owner.poolState().linkedCount==1024);for(auto* n:nodes)check(n->entries!=0);if(!round)firstBlocks=owner.blockCount();else check(owner.blockCount()==firstBlocks);
unsigned processed=0,resets=0;
studioMaterialQueueDrainCandidate(linkedQueue,[&](auto* call){check(call==queue[processed]&&call->argument==reinterpret_cast<uintptr_t>(nodes[processed]));call->invoke();++processed;check(owner.activeArrayCount()==1024-processed&&owner.poolState().linkedCount==1024-processed);},[&](auto* call){check(call->releaseReference([&](auto* p){++jobFrees;delete p;})==0);},[&](bool decommit){check(!decommit&&linkedQueue.head&&linkedQueue.tail);++resets;});check(processed==1024&&resets==1&&!linkedQueue.head&&!linkedQueue.tail);
studioMaterialQueueDrainCandidate(linkedQueue,[](auto*){throw std::runtime_error("empty invoke");},[](auto*){throw std::runtime_error("empty release");},[](bool){throw std::runtime_error("empty reset");});
check(!owner.poolState().head&&!owner.poolState().tail&&owner.activeArrayCount()==0);unsigned freeCount=0;for(auto* n=owner.poolState().freeHead;n;n=n->next){check(n->previous==n&&!n->entries);check(++freeCount<=1024);}check(freeCount==1024);
}
check(releases==2048&&jobFrees==2048);std::cout<<"deferred_resource_releases=2048 job_frees=2048 stable_blocks="<<firstBlocks<<"\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
