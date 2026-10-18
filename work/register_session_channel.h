#pragma once
#include "session_runtime.h"
#include "operator_attribute_registration.h"
// Requires ChannelBinding; primary operator slots +0x10/+0x14 contracts.
inline size_t registerSessionChannel(SessionRuntime&r,OperatorAttributeRegistration&registry,uint32_t channel,const ChannelBinding&binding){
 auto endpoint=[&](const ChannelEndpoint&e)->uintptr_t{if(!e.present())return 0;auto owner=r.elements.graphHandles.at(e.element);auto handle=r.attributes.find(owner,e.attribute);return r.attributes.identity(r.graph,r.elements,handle);};
 auto owner=r.elements.graphHandles.at(channel);auto identity=r.elements.table.resolve(owner);if(!identity)throw std::runtime_error("retired channel registration");
 auto source=endpoint(binding.source),destination=endpoint(binding.destination);std::vector<uintptr_t> inputs,outputs;
 if(binding.mode!=0&&binding.mode!=3&&source)inputs.push_back(source);
 if(binding.mode!=0&&destination&&!((binding.mode==1||binding.mode==2)&&source==destination))outputs.push_back(destination);
 return registry.add(identity,inputs,outputs);
}
