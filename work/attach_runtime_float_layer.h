#pragma once
#include "session_runtime.h"
// Graph/reference portion of 0x1051A7BE..0x1051A7DC. Native mutation
// notifications, undo and transient owner pointer are not implemented here.
inline bool attachRuntimeFloatLayer(SessionRuntime&r,uint32_t log,uint32_t layer){
 auto owner=r.elements.resolve(r.graph,log);auto target=r.elements.resolve(r.graph,layer);
 if(!owner||!target||r.graph.strings.at(owner->type)!="DmeFloatLog"||r.graph.strings.at(target->type)!="DmeFloatLogLayer")return false;
 auto handle=r.attributes.find(log,"layers");auto attribute=r.attributes.resolve(r.graph,r.elements,handle);
 if(!attribute||attribute->type!=15)return false;
 auto record=reinterpret_cast<const SessionElementRecord*>(r.elements.table.resolve(layer));
 // Reserve both containers before publishing either side of the reference.
 attribute->values.reserve(attribute->values.size()+1);
 auto&incoming=r.references.incoming[layer];incoming.reserve(incoming.size()+1);
 Value value;value.reference=record->graphIndex;attribute->values.push_back(std::move(value));
 r.references.link(layer,handle);return true;
}
