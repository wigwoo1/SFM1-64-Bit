#pragma once
#include "session_runtime.h"
#include "mutation_coordinator.h"
#include "float_curveinfo_schema.h"
#include "attribute_initial_flags.h"
// Recovered graph/schema construction; native factory/file/undo lifecycle separate.
inline uint32_t createRuntimeFloatCurveInfo(SessionRuntime&r,MutationCoordinator&m,const std::array<char,16>&id,const std::string&name,uint8_t ownerState){
 for(const auto&e:r.graph.elements)if(e.id==id)throw std::runtime_error("duplicate curveinfo UUID");
 Element e;e.id=id;e.type=internString(r.graph,"DmeFloatCurveInfo");e.name=internString(r.graph,name);
 for(const auto&schema:floatCurveInfoSchemaRecovered){Attribute a;a.name=internString(r.graph,schema.name);a.type=schema.type;Value value;value.wire.assign(schema.type==4?1:4,0);if(std::string(schema.name)=="maxvalue")setWireU32(value,0x3f800000);a.values.push_back(std::move(value));e.attributes.push_back(std::move(a));}
 Attribute n;n.name=internString(r.graph,"name");n.type=5;Value text;text.text=name;text.stringIndex=e.name;n.values.push_back(std::move(text));e.attributes.push_back(std::move(n));
 auto index=static_cast<uint32_t>(r.graph.elements.size());r.graph.elements.push_back(std::move(e));auto handle=r.elements.bind(r.graph,index);auto&owner=m.owners[handle];owner.state=ownerState;
 for(const auto&a:r.graph.elements[index].attributes){auto nameText=r.graph.strings.at(a.name);auto attr=r.attributes.bind(r.graph,r.elements,handle,nameText);owner.attributeFlags[r.attributes.identity(r.graph,r.elements,attr)]=static_cast<uint16_t>(initialAttributeFlags(a.type)|(nameText=="name"?0x400:0));}return handle;
}
