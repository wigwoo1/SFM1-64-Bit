#include <vector>
#include <iostream>
#include <stdexcept>
#include "studio_decal_queued_call_candidate.h"
static uintptr_t receivedOwner,receivedArgument;static unsigned invocations;
static void method(uintptr_t owner,uintptr_t argument){receivedOwner=owner;receivedArgument=argument;++invocations;}
static void check(bool v){if(!v)throw std::runtime_error("queued call contract");}
int main(){try{std::vector<StudioDecalQueuedCallCandidate*> queue;for(unsigned i=0;i<4096;++i){uintptr_t argument=uintptr_t(0x100000000ULL+i);studioDecalQueueCallCandidate(uintptr_t(0x200000000ULL+i),&method,&argument,[]{return new StudioDecalQueuedCallCandidate;},[&](auto* call){queue.push_back(call);});argument=0;check(queue.back()->argument==uintptr_t(0x100000000ULL+i));}unsigned frees=0;for(unsigned i=0;i<queue.size();++i){auto* call=queue[i];check(call->addReference()==2);check(call->releaseReference([&](auto*){throw std::runtime_error("premature free");})==1);call->invoke();check(receivedOwner==uintptr_t(0x200000000ULL+i)&&receivedArgument==uintptr_t(0x100000000ULL+i));check(call->releaseReference([&](auto* p){++frees;delete p;})==0);}check(frees==4096&&invocations==4096);unsigned nullEnqueues=0;studioDecalQueueCallCandidate(0,&method,nullptr,[]()->StudioDecalQueuedCallCandidate*{return nullptr;},[&](auto* p){check(!p);++nullEnqueues;});check(nullEnqueues==1);std::cout<<"queued_calls=4096 frees=4096 null_enqueue=1\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
