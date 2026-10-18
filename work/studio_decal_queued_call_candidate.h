#pragma once
#include <atomic>
#include <cstdint>
struct StudioDecalQueuedCallCandidate {
 using Method=void(*)(uintptr_t,uintptr_t);
 std::atomic<uint32_t> references{1};
 Method method=nullptr;uintptr_t owner=0,argument=0;
 void invoke(){method(owner,argument);}
 uint32_t addReference(){return references.fetch_add(1)+1;}
 template<class Free> uint32_t releaseReference(Free freeObject){uint32_t remaining=references.fetch_sub(1)-1;if(!remaining)freeObject(this);return remaining;}
};
//100572E0. Queue ownership policy is supplied, not inferred. Null allocation
// still reaches enqueue; original argument is copied only on successful allocation.
template<class Allocate,class Enqueue>
inline void studioDecalQueueCallCandidate(uintptr_t owner,StudioDecalQueuedCallCandidate::Method method,const uintptr_t* argument,Allocate allocate,Enqueue enqueue){
 auto* call=allocate();if(call){call->references.store(1);call->method=method;call->owner=owner;call->argument=*argument;}enqueue(call);
}
