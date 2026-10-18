#pragma once
#include "studio_decal_queued_call_candidate.h"
struct StudioMaterialQueueNodeCandidate {StudioMaterialQueueNodeCandidate* next=nullptr;StudioDecalQueuedCallCandidate* call=nullptr;};
struct StudioMaterialQueueCandidate {StudioMaterialQueueNodeCandidate* head=nullptr;StudioMaterialQueueNodeCandidate* tail=nullptr;};
//10005320 linked append. Node allocation/buffer cursor remain supplied.
template<class Queue,class Node>
inline void studioMaterialQueueAppendCandidate(Queue& queue,Node* node){if(queue.tail)queue.tail->next=node;else queue.head=node;queue.tail=node;node->next=nullptr;}
//100219A0. Node storage must survive call release. Load next after callbacks,
// allowing a callback to append to the live tail, as the original routine does.
template<class Queue,class Invoke,class Release,class Reset>
inline void studioMaterialQueueDrainCandidate(Queue& queue,Invoke invoke,Release release,Reset resetBuffer){
 auto* node=queue.head;if(!node)return;
 do{auto* call=node->call;invoke(call);release(call);node=node->next;}while(node);
 resetBuffer(false);queue.head=queue.tail=nullptr;
}
