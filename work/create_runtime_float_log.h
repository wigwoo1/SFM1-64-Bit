#pragma once
#include "create_runtime_float_layer.h"
#include "float_log_runtime_metadata.h"
#include "log_attribute_defaults.h"
#include "attach_mutating_float_layer.h"
#include "runtime_float_layer_state.h"
// Recovered graph construction with explicit UUIDs and post-create owner state.
// Native factory queues, file ownership, undo and full inherited schema omitted.
inline uint32_t createRuntimeFloatLog(SessionRuntime&r,MutationCoordinator&m,const std::array<char,16>&logId,const std::array<char,16>&layerId,uint8_t ownerState,RuntimeFloatLayerState*layerState=nullptr){
 if(logId==layerId)throw std::runtime_error("log/layer UUID collision");
 for(const auto&e:r.graph.elements)if(e.id==logId||e.id==layerId)throw std::runtime_error("duplicate creation UUID");
 if(!attributeMutationPermitted(0xd0f,m.cycle.phase))throw std::runtime_error("initial layer attachment unavailable in phase");
 Element e;e.id=logId;e.type=internString(r.graph,"DmeFloatLog");e.name=internString(r.graph,"float log");
 e.attributes.push_back(makeDefaultLogArrayAttribute(internString(r.graph,"layers"),15));
 e.attributes.push_back(makeDefaultLogReferenceAttribute(internString(r.graph,"curveinfo")));
 Attribute used;used.name=internString(r.graph,"usedefaultvalue");used.type=4;Value no;no.wire.assign(1,0);used.values.push_back(std::move(no));e.attributes.push_back(std::move(used));
 e.attributes.push_back(makeDefaultFloatAttribute(internString(r.graph,"defaultvalue")));
 e.attributes.push_back(makeDefaultLogArrayAttribute(internString(r.graph,"bookmarks"),21));
 Attribute name;name.name=internString(r.graph,"name");name.type=5;Value text;text.text="float log";text.stringIndex=e.name;name.values.push_back(std::move(text));e.attributes.push_back(std::move(name));
 auto index=static_cast<uint32_t>(r.graph.elements.size());r.graph.elements.push_back(std::move(e));auto handle=r.elements.bind(r.graph,index);
 for(const auto&a:r.graph.elements[index].attributes)r.attributes.bind(r.graph,r.elements,handle,r.graph.strings.at(a.name));
 if(!bindFloatLogRuntimeMetadata(r,m,handle,ownerState))throw std::runtime_error("created log metadata mismatch");
 auto nameHandle=r.attributes.find(handle,"name");m.owners.at(handle).attributeFlags[r.attributes.identity(r.graph,r.elements,nameHandle)]=0x405;
 auto layer=createRuntimeFloatLayer(r,m,layerId,ownerState);
 if(layerState){layerState->construct(layer);layerState->attach(layer,handle);}
 if(!attachMutatingFloatLayer(r,m,handle,layer,false,[](unsigned,uint32_t,size_t,size_t){},[](){return false;}))throw std::runtime_error("initial layer attachment failed");
 return handle;
}


