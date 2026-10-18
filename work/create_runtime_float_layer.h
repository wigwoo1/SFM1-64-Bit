#pragma once
#include "session_runtime.h"
#include "mutation_coordinator.h"
#include "float_layer_attribute_defaults.h"
#include "attribute_initial_flags.h"
// Isolated construction primitive. Caller supplies UUID and explicit post-create
// owner state; native creation queues/undo/file ownership remain separate.
inline uint32_t createRuntimeFloatLayer(SessionRuntime&r,MutationCoordinator&m,const std::array<char,16>&uuid,uint8_t ownerState){
 for(const auto&e:r.graph.elements)if(e.id==uuid)throw std::runtime_error("duplicate layer UUID");
 Element e;e.id=uuid;e.type=internString(r.graph,"DmeFloatLogLayer");e.name=internString(r.graph,"float log");e.attributes=makeFloatLayerAttributeDefaults(r.graph);
 Attribute name;name.name=internString(r.graph,"name");name.type=5;Value value;value.text="float log";value.stringIndex=e.name;name.values.push_back(std::move(value));e.attributes.push_back(std::move(name));
 auto index=static_cast<uint32_t>(r.graph.elements.size());r.graph.elements.push_back(std::move(e));auto handle=r.elements.bind(r.graph,index);auto&owner=m.owners[handle];owner.state=ownerState;
 for(const auto&a:r.graph.elements[index].attributes){auto text=r.graph.strings[a.name];auto attr=r.attributes.bind(r.graph,r.elements,handle,text);auto identity=r.attributes.identity(r.graph,r.elements,attr);owner.attributeFlags[identity]=initialAttributeFlags(a.type)|(text=="compressed"?0x8000:text=="name"?0x400:0);}
 return handle;
}
