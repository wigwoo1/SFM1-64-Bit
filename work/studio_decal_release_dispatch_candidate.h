#pragma once
#include <cstdint>
//100580F0 slot34. Context must be non-null: the original conditionally calls
// slot2, then unconditionally reads slots143/3/1. No invented null fallback.
// x64 exception policy balances acquired context if enter throws. Enter must
// roll back an uncompleted scope; release must not throw. Later failures remain
// outside this narrow cleanup guarantee.
// getOwner is called after queue/config checks, matching the original global load.
template<class Context,class Enter,class Queue,class Enabled,class Owner,class Enqueue,class Direct,class Leave,class Release>
inline void studioDecalReleaseDispatchCandidate(uintptr_t argument,Context getContext,Enter enter,Queue getQueue,Enabled enabled,Owner getOwner,Enqueue enqueue,Direct direct,Leave leave,Release release){
 auto context=getContext();if(context){try{enter(context);}catch(...){release(context);throw;}}auto queue=getQueue(context);
 if(queue&&enabled())enqueue(queue,getOwner(),&argument);else direct(getOwner(),argument);
 leave(context);release(context);
}

