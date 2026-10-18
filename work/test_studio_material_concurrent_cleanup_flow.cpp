#define NOMINMAX
#include <windows.h>
#include <thread>
#include <vector>
#include <atomic>
#include <iostream>
#include "studio_material_queued_context_control_candidate.h"
#include "studio_material_immediate_lock_candidate.h"
#include "studio_material_contended_lock_candidate.h"
#include "studio_decal_resource_owner_candidate.h"
struct Shared{StudioMaterialImmediateLockCandidate lock;unsigned cleaned=0;std::atomic<unsigned> collisions{0},failures{0};};
struct Receiver{StudioDecalResourceOwnerCandidate* owner;Shared* shared;};
static void begin(uintptr_t receiver){auto& shared=*reinterpret_cast<Shared*>(receiver);studioMaterialImmediateBeginCandidate(shared.lock,GetCurrentThreadId(),[&](auto& lock,uint32_t thread,int parameter){if(parameter)++shared.failures;++shared.collisions;studioMaterialContendedLockCandidate(lock,thread,parameter,[]{YieldProcessor();},[](int milliseconds){Sleep(DWORD(milliseconds));},[]{return GetThreadPriority(GetCurrentThread());});});}
static void end(uintptr_t receiver){auto& shared=*reinterpret_cast<Shared*>(receiver);if(shared.lock.owner.load()!=GetCurrentThreadId()||shared.lock.depth!=1)++shared.failures;studioMaterialImmediateEndCandidate(shared.lock);}
static void cleanup(uintptr_t receiver,uintptr_t argument){auto& state=*reinterpret_cast<Receiver*>(receiver);auto& shared=*state.shared;if(shared.lock.owner.load()!=GetCurrentThreadId()||shared.lock.depth!=1)++shared.failures;
auto before=shared.cleaned;std::this_thread::yield();state.owner->release(reinterpret_cast<StudioDecalPointerNodeCandidate*>(argument),[](auto*){},[](uint16_t){throw 1;},[](uintptr_t){throw 1;});shared.cleaned=before+1;}
int main(){Shared shared;std::atomic<unsigned> ready{0};std::vector<std::thread> workers;
for(unsigned worker=0;worker<4;++worker)workers.emplace_back([&]{StudioDecalResourceOwnerCandidate owner;Receiver receiver{&owner,&shared};StudioMaterialQueuedContextControlCandidate context(reinterpret_cast<uintptr_t>(&shared),begin,end);
for(unsigned i=0;i<256;++i){int entries=1+int(i%8);auto* resource=owner.create(0x100000001ULL+i,[&]{return entries;});context.begin();context.begin();uintptr_t argument=reinterpret_cast<uintptr_t>(resource);studioDecalQueueCallCandidate(reinterpret_cast<uintptr_t>(&receiver),cleanup,&argument,[]{return new StudioDecalQueuedCallCandidate;},[&](auto* call){context.enqueue(call);});argument=0;context.end();context.end();}
if(owner.activeArrayCount()!=256||context.queue().queuedCount()!=768||context.nestingCount())++shared.failures;
++ready;while(ready.load()!=4)std::this_thread::yield();context.flush();if(owner.activeArrayCount()||owner.poolState().linkedCount||context.queue().queuedCount()||context.referenceCount()!=1)++shared.failures;
});
for(auto& worker:workers)worker.join();if(shared.failures||shared.cleaned!=1024||shared.lock.owner.load()||shared.lock.depth)return 1;
std::cout<<"concurrent_queue_drains=4 actual_studio_releases="<<shared.cleaned<<" mixed_calls=3072 contention_callbacks="<<shared.collisions<<" final_unlocked=1\n";}
