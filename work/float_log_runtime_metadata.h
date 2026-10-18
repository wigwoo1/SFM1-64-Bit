#pragma once
#include "session_runtime.h"
#include "mutation_coordinator.h"
#include "float_log_schema.h"
#include "attribute_initial_flags.h"
// Registers only recovered float-log schema fields. Owner state is supplied
// explicitly; this is not a complete native deserialization lifecycle.
inline bool bindFloatLogRuntimeMetadata(SessionRuntime&r,MutationCoordinator&m,uint32_t log,uint8_t state){
 auto element=r.elements.resolve(r.graph,log);
 if(!element||r.graph.strings.at(element->type)!="DmeFloatLog")return false;
 std::map<uintptr_t,uint16_t> flags;
 for(const auto&schema:floatLogSchemaRecovered){
  auto handle=r.attributes.find(log,schema.name);auto attribute=r.attributes.resolve(r.graph,r.elements,handle);
  if(!attribute||attribute->type!=schema.type)return false;
  flags.emplace(r.attributes.identity(r.graph,r.elements,handle),static_cast<uint16_t>(initialAttributeFlags(schema.type)|schema.addedFlags));
 }
 if(m.owners.count(log))return false;
 auto&owner=m.owners[log];owner.state=state;owner.attributeFlags=std::move(flags);return true;
}

