#pragma once
#include "session_vector_runtime.h"
#include "session_quaternion_runtime.h"
#include "runtime_channel_play.h"
// Reuse recovered channel metadata and generation-checked endpoint caches.
using RuntimeCachedTypedPlayBinding=RuntimeCachedFloatPlayBinding;
inline bool executeRuntimeTypedPlay(Graph&g,const SessionRuntimeHandles&r,const SessionAttributeHandles&a,RuntimeCachedTypedPlayBinding&b,int32_t tick,SessionAttributeChanges*changes=nullptr,bool force=false,const std::vector<std::pair<bool,bool>>*infinite=nullptr,const std::array<float,3>*disabledVectorEdge=nullptr){
 if(b.saved.destination.scalarType==3)return executeRuntimeFloatPlay(g,r,a,b,tick,changes,force,infinite);
 if(!r.resolve(g,b.channelHandle)||!r.resolve(g,b.destinationHandle)||!r.resolve(g,b.logHandle))return false;
 for(auto layer:b.layerHandles)if(!r.resolve(g,layer))return false;
 if(!cachedPlayMetadataCurrent(g,b))return false;
 auto*destination=a.resolve(g,r,b.destinationAttribute);if(!destination){b.destinationAttribute=a.find(b.destinationHandle,b.saved.destination.attribute);destination=a.resolve(g,r,b.destinationAttribute);}
 auto type=b.saved.destination.scalarType;
 if(!destination||(destination->type!=type&&destination->type!=type+14)||b.saved.destination.index<0||size_t(b.saved.destination.index)>=destination->values.size())return false;
 if(type==10)return executeSessionVectorPlay(g,b.saved,tick,changes,infinite,disabledVectorEdge);
 if(type==13)return executeSessionQuaternionPlay(g,b.saved,tick,changes,infinite);
 return false;
}
