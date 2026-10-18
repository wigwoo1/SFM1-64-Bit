#pragma once
#include "session_runtime.h"
struct ChannelEndpointCache{uint32_t source=0xffffffff,destination=0xffffffff;};
// Native element-interface callback 0x104CC960. Index, mode and log edits
// do not invalidate these endpoint caches through this callback.
inline bool invalidateChannelEndpointCache(SessionRuntime&r,uint32_t channel,uint32_t changed,ChannelEndpointCache&cache){
 auto element=r.elements.resolve(r.graph,channel);if(!element||r.graph.strings.at(element->type)!="DmeChannel")return false;
 if(!r.attributes.resolve(r.graph,r.elements,changed))return false;
 if(changed==r.attributes.find(channel,"fromElement")||changed==r.attributes.find(channel,"fromAttribute")){cache.source=0xffffffff;return true;}
 if(changed==r.attributes.find(channel,"toElement")||changed==r.attributes.find(channel,"toAttribute")){cache.destination=0xffffffff;return true;}
 return false;
}
