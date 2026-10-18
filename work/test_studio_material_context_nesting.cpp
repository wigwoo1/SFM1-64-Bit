#include <vector>
#include <iostream>
#include "studio_material_queue_owner_candidate.h"
#include "studio_material_context_nesting_candidate.h"
static std::vector<unsigned> events;
static void invoke(uintptr_t,uintptr_t argument){events.push_back(unsigned(argument));}
int main(){StudioMaterialQueueOwnerCandidate queue;unsigned cases=0;
for(unsigned n=1;n<=128;++n){uint32_t depth=0;events.clear();auto enqueue=[&](unsigned tag){auto* call=new StudioDecalQueuedCallCandidate;call->method=invoke;call->argument=tag;queue.enqueue(call);};
for(unsigned i=0;i<n;++i)studioMaterialBeginCandidate(depth,[&]{enqueue(1);});
enqueue(2);
for(unsigned i=0;i<n;++i)studioMaterialEndCandidate(depth,[&]{enqueue(3);});
if(depth||queue.queuedCount()!=3||!events.empty())return 1;
queue.drain();if(events!=std::vector<unsigned>{1,2,3}||queue.queuedCount())return 1;++cases;
}
std::cout<<"nested_context_cases="<<cases<<" ordered_calls="<<cases*3<<"\n";}
