#include <vector>
#include <array>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include "studio_material_mixed_queue_owner_candidate.h"
static StudioMaterialMixedQueueOwnerCandidate* active;static unsigned failure;static bool appendOnFailure;static std::vector<unsigned> events;
static void appended(uintptr_t){events.push_back(3);}
static void invoke(uintptr_t,uintptr_t id){events.push_back(static_cast<unsigned>(id));if(id==failure){if(appendOnFailure)active->enqueueZero(0,appended);throw std::runtime_error("callback failure");}}
static void check(bool x){if(!x)std::abort();}
int main(){for(bool append:{false,true})for(failure=0;failure<3;++failure){appendOnFailure=append;events.clear();StudioMaterialMixedQueueOwnerCandidate queue;active=&queue;std::array<StudioDecalQueuedCallCandidate*,3> calls{};
 for(unsigned i=0;i<3;++i){calls[i]=new StudioDecalQueuedCallCandidate;calls[i]->method=invoke;calls[i]->argument=i;calls[i]->addReference();queue.enqueue(calls[i]);}
 bool threw=false;try{queue.drain();}catch(const std::runtime_error&){threw=true;}
 check(threw&&queue.queuedCount()==2-failure+unsigned(append));
 for(unsigned i=0;i<3;++i)check(calls[i]->references.load()==(i<=failure?1u:2u));
 auto saved=failure;failure=99;queue.drain();failure=saved;std::vector<unsigned> expected{0,1,2};if(append)expected.push_back(3);check(events==expected&&queue.queuedCount()==0);
 for(auto* call:calls){check(call->references.load()==1);call->releaseReference([](auto* p){delete p;});}
 queue.enqueueZero(0,appended);queue.drain();check(events.back()==3&&events.size()==4+unsigned(append));
 }std::puts("material_mixed_queue_throwing_callbacks_cases=6 pending_suffix_reference_cleanup_and_reuse=passed");}


