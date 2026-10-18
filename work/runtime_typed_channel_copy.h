#pragma once
#include "runtime_typed_channel_play.h"
#include "color_write_sequence.h"
using RuntimeCachedTypedCopyBinding=RuntimeCachedFloatCopyBinding;
inline bool executeRuntimeTypedCopy(Graph&g,const SessionRuntimeHandles&r,const SessionAttributeHandles&a,RuntimeCachedTypedCopyBinding&b,SessionAttributeChanges*changes=nullptr){
 if(b.saved.destination.scalarType==3)return executeRuntimeFloatCopy(g,r,a,b,changes);
 if(b.saved.mode!=1||!r.resolve(g,b.channelHandle)||!r.resolve(g,b.sourceHandle)||!r.resolve(g,b.destinationHandle))return false;
 auto resolve=[&](uint32_t&cache,uint32_t owner,const std::string&name){auto*p=a.resolve(g,r,cache);if(!p){cache=a.find(owner,name);p=a.resolve(g,r,cache);}return p;};
 auto*source=resolve(b.sourceAttribute,b.sourceHandle,b.saved.source.attribute);auto*destination=resolve(b.destinationAttribute,b.destinationHandle,b.saved.destination.attribute);
 if(!source||!destination||!runtimeChannelSnapshotCurrent(g,b))return false;
 auto type=b.saved.destination.scalarType;if(type!=b.saved.source.scalarType||(type!=8&&type!=10&&type!=13))return false;
 if(b.sourceHandle==b.destinationHandle&&b.saved.source.attribute==b.saved.destination.attribute)return true;
 auto&wire=source->values.at(b.saved.source.index).wire;size_t width=type==8?4:type==10?12:16;if(wire.size()!=width)return false;
 if(type==8){auto&target=destination->values.at(b.saved.destination.index).wire;if(target.size()!=4)return false;uint32_t old,incoming;std::memcpy(&old,target.data(),4);std::memcpy(&incoming,wire.data(),4);bool changed=writeRecoveredColorCandidate(old,incoming,[](){return true;},[](){},[](){},[](){});if(changed){std::memcpy(target.data(),&old,4);if(changes)changes->attributes.emplace_back(b.saved.destination.element,b.saved.destination.attribute);}}
 else if(type==10){std::array<float,3>v;std::memcpy(v.data(),wire.data(),12);runtimeWriteVector(g,b.saved.destination,v,changes);}
 else{std::array<float,4>v;std::memcpy(v.data(),wire.data(),16);runtimeWriteQuaternion(g,b.saved.destination,v,changes);}return true;
}
