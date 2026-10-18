#pragma once
#include "session_mutation_dispatch.h"
#include "channel_activity.h"
struct SessionOperatorActivity{
 SessionMutationDispatch& dispatch;
 MutationOwner* owner(uint32_t handle){if(!dispatch.live(handle))return nullptr;auto it=dispatch.mutation.owners.find(handle);return it==dispatch.mutation.owners.end()?nullptr:&it->second;}
 bool expression(uint32_t handle){auto record=owner(handle);return record&&forcedOperatorActive(record->state);}
 bool channel(uint32_t handle,int32_t mode,uint32_t cachedC4,bool sourceResolved,uint16_t sourceFlags){auto record=owner(handle);return record&&channelActive(record->state,mode,cachedC4,sourceResolved,sourceFlags);}
 // Resolves cached source attribute identity; name-based fallback belongs to caller.
 bool channelSource(uint32_t handle,int32_t mode,uint32_t cachedC4,uint32_t sourceHandle){
  auto&r=dispatch.runtime;auto identity=r.attributes.identity(r.graph,r.elements,sourceHandle);
  uint16_t flags=0;bool resolved=false;
  if(identity){auto source=reinterpret_cast<const SessionAttributeRecord*>(identity);auto metadata=owner(source->ownerHandle);if(metadata){auto found=metadata->attributeFlags.find(identity);if(found!=metadata->attributeFlags.end()){flags=found->second;resolved=true;}}}
  return channel(handle,mode,cachedC4,resolved,flags);
 }
 bool prepare(uint32_t handle){auto record=owner(handle);if(!record)return false;prepareChannelActivity(record->state);return true;}
 bool channelSourceCached(uint32_t handle,int32_t mode,uint32_t cachedC4,uint32_t sourceOwner,const std::string&name,uint32_t&sourceCache){
  // Native forced/play/cache branches return before resolving source.
  if(channel(handle,mode,cachedC4,false,0))return true;
  if(!owner(handle)||(mode!=1&&mode!=2))return false;
  auto&r=dispatch.runtime;
  if(!r.attributes.resolve(r.graph,r.elements,sourceCache))sourceCache=r.attributes.find(sourceOwner,name);
  return channelSource(handle,mode,cachedC4,sourceCache);
 }
};
