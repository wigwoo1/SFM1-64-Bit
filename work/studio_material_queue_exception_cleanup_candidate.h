#pragma once
#include "studio_material_queue_drain_candidate.h"
// x64 exception policy, beyond the original routine's verified normal path.
// Release and reset callbacks must not throw. No recursive drain is supported.
// An invoked callback is consumed even if it throws; remaining work stays queued.
template<class Queue,class Invoke,class Release,class Reset>
inline void studioMaterialQueueDrainWithExceptionCleanupCandidate(Queue& queue,Invoke invoke,Release release,Reset resetBuffer){
 auto* node=queue.head;if(!node)return;
 do{
  auto* call=node->call;
  try{invoke(call);}catch(...){
   release(call);queue.head=node->next;
   if(!queue.head){resetBuffer(false);queue.tail=nullptr;}
   throw;
  }
  release(call);node=node->next;
 }while(node);
 resetBuffer(false);queue.head=queue.tail=nullptr;
}
