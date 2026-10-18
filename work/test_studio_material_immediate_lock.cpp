#define NOMINMAX
#include <windows.h>
#include <thread>
#include <vector>
#include <atomic>
#include <iostream>
#include "studio_material_immediate_lock_candidate.h"
int main(){StudioMaterialImmediateLockCandidate lock;std::atomic<unsigned> ready{0},failures{0},contentions{0};unsigned protectedCount=0;std::vector<std::thread> workers;
for(unsigned i=0;i<4;++i)workers.emplace_back([&]{auto thread=GetCurrentThreadId();++ready;while(ready.load()!=4)std::this_thread::yield();for(unsigned n=0;n<4096;++n){auto contended=[&](auto& state,uint32_t identity,int parameter){if(parameter)++failures;++contentions;for(;;){uint32_t expected=0;if(state.owner.compare_exchange_weak(expected,identity)){++state.depth;return;}std::this_thread::yield();}};
studioMaterialImmediateBeginCandidate(lock,thread,contended);studioMaterialImmediateBeginCandidate(lock,thread,contended);if(lock.owner.load()!=thread||lock.depth!=2)++failures;auto before=protectedCount;std::this_thread::yield();protectedCount=before+1;studioMaterialImmediateEndCandidate(lock);if(lock.depth!=1||lock.owner.load()!=thread)++failures;studioMaterialImmediateEndCandidate(lock);}});
for(auto& worker:workers)worker.join();if(failures||protectedCount!=16384||lock.depth||lock.owner.load())return 1;
std::cout<<"recursive_lock_threads=4 protected_updates="<<protectedCount<<" nested_entries=32768 contentions="<<contentions<<" final_unlocked=1\n";}
