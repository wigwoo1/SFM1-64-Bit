#pragma once
#include "coordinated_reference_write.h"
#include "channel_endpoint_cache.h"
#include "session_reference_propagation.h"
#include "recovered_log_classes.h"
// Integrates recovered reference mutation, endpoint callback and propagation.
// Schema type acceptance, undo and framework dirty fields remain separate.
template<class Suppressed>
bool writeChannelReference(SessionRuntime&r,MutationCoordinator&m,uint32_t channel,const std::string&name,uint32_t target,ChannelEndpointCache&cache,bool preparationSuppressed,Suppressed propagationSuppressed){
 auto e=r.elements.resolve(r.graph,channel);if(!e||r.graph.strings.at(e->type)!="DmeChannel")return false;
 if(name!="fromElement"&&name!="toElement"&&name!="log")return false;
 if(name=="log"){
  if(!coordinatedScalarReferenceWriteChecked(r,m,channel,name,target,preparationSuppressed,[&](uint32_t candidate){
   auto element=r.elements.resolve(r.graph,candidate);const auto&type=r.graph.strings.at(element->type);
   // Proven 0x10500420 ancestry. Other log classes need their own recovery;
   // surface unsupported validation instead of declaring them incompatible.
   if(recoveredLogClass(type))return true;
   if(type=="DmeChannel"||type=="DmeFloatLogLayer")return false;
   throw std::runtime_error("unrecovered channel log target ancestry: "+type);
  }))return false;
 }else if(!coordinatedReferenceWrite(r,m,channel,name,0,target,preparationSuppressed))return false;
 auto a=r.attributes.find(channel,name);auto identity=r.attributes.identity(r.graph,r.elements,a);auto&owner=m.owners.at(channel);
 if((owner.attributeFlags.at(identity)&0x100)&&(owner.state&4))invalidateChannelEndpointCache(r,channel,a,cache);
 propagateSessionReferences(r,m,channel,3,propagationSuppressed);return true;
}

