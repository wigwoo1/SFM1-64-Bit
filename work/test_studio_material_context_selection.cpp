#include <thread>
#include <vector>
#include <atomic>
#include <iostream>
#include "studio_material_context_selection_candidate.h"
static thread_local uintptr_t active;
int main(){std::atomic<unsigned> failures{0},cycles{0};std::vector<std::thread> workers;
 for(unsigned worker=0;worker<4;++worker)workers.emplace_back([&,worker]{
  uintptr_t immediate=0x100000000ULL+worker*4096,queued=immediate+256;unsigned calls=0;uintptr_t expected=0;
  auto get=[](){return active;};auto set=[](uintptr_t value){active=value;};
  auto slot0=[&](uintptr_t value){if(value!=expected)++failures;++calls;};
  for(unsigned i=0;i<1024;++i){
   expected=immediate;if(studioMaterialGetContextCandidate(immediate,get,set,slot0)!=immediate||active!=immediate)++failures;
   expected=queued;if(studioMaterialSetContextCandidate(queued,get,set,slot0)!=immediate||active!=queued)++failures;
   if(studioMaterialGetContextCandidate(immediate,get,set,slot0)!=queued)++failures;
   if(studioMaterialSetContextCandidate(0,get,set,slot0)!=queued||active)++failures;
   ++cycles;
  }
  if(calls!=3072)++failures;
 });
 for(auto& worker:workers)worker.join();
 if(failures||active)return 1;
 std::cout<<"context_selection_cycles="<<cycles<<" isolated_threads=4 slot0_calls=12288\n";
}
