#include <vector>
#include <iostream>
#include "studio_material_queued_context_control_candidate.h"
#include "studio_material_context_selection_candidate.h"
#include "studio_decal_release_dispatch_candidate.h"
#include "studio_decal_resource_owner_candidate.h"
static thread_local uintptr_t active;
static std::vector<unsigned> events;
static void begin(uintptr_t owner){if(owner!=0x100000001ULL)throw 1;events.push_back(1);}
static void end(uintptr_t owner){if(owner!=0x100000001ULL)throw 1;events.push_back(3);}
static void cleanup(uintptr_t owner,uintptr_t argument){events.push_back(2);reinterpret_cast<StudioDecalResourceOwnerCandidate*>(owner)->release(reinterpret_cast<StudioDecalPointerNodeCandidate*>(argument),[](auto*){},[](uint16_t){throw 1;},[](uintptr_t){throw 1;});}
int main(){unsigned cases=0;
for(unsigned count=1;count<=128;++count)for(unsigned enabled=0;enabled<2;++enabled){StudioMaterialQueuedContextControlCandidate context(0x100000001ULL,begin,end);StudioDecalResourceOwnerCandidate owner;auto identity=reinterpret_cast<uintptr_t>(&context);auto get=[](){return active;};auto set=[](uintptr_t value){active=value;};auto retain=[](uintptr_t value){reinterpret_cast<StudioMaterialQueuedContextControlCandidate*>(value)->addReference();};
if(studioMaterialSetContextCandidate(identity,get,set,retain)||context.referenceCount()!=2)return 1;
int entries=int(count);auto* resource=owner.create(0x100000001ULL,[&]{return entries;});events.clear();
studioDecalReleaseDispatchCandidate(reinterpret_cast<uintptr_t>(resource),[&]{return studioMaterialGetContextCandidate(identity,get,set,retain);},[&](uintptr_t value){if(value!=identity)throw 1;context.begin();},[&](uintptr_t){return reinterpret_cast<uintptr_t>(&context.queue());},[&]{return enabled!=0;},[&]{return reinterpret_cast<uintptr_t>(&owner);},[&](uintptr_t value,uintptr_t receiver,const uintptr_t* argument){if(value!=reinterpret_cast<uintptr_t>(&context.queue()))throw 1;studioDecalQueueCallCandidate(receiver,cleanup,argument,[]{return new StudioDecalQueuedCallCandidate;},[&](auto* call){context.enqueue(call);});},cleanup,[&](uintptr_t){context.end();},[&](uintptr_t){if(context.releaseReference([](auto*){throw 1;})!=2)throw 1;});
if(context.nestingCount()||context.referenceCount()!=2||context.queue().queuedCount()!=(enabled?3:2)||owner.activeArrayCount()!=(enabled?1:0))return 1;
context.flush();if(events!=(enabled?std::vector<unsigned>{1,2,3}:std::vector<unsigned>{2,1,3})||context.queue().queuedCount()||owner.activeArrayCount())return 1;
if(studioMaterialSetContextCandidate(0,get,set,retain)!=identity||active)return 1;
if(context.releaseReference([](auto*){throw 1;})!=1)return 1;++cases;
}std::cout<<"selected_context_cleanup_cases="<<cases<<" queued=128 direct=128 balanced_references=1\n";}
