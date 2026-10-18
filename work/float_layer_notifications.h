#pragma once
#include "session_runtime.h"
// DmeFloatLogLayer vtable 10B9202C: slot 0=101F3E50 (ret 4),
// slots 4/8=10129D00 (ret 0Ch). No native class-specific side effects.
inline void notifyRecoveredFloatLayer(SessionRuntime&r,unsigned slot,uint32_t attribute,int32_t,int32_t){
 if(slot!=0&&slot!=4&&slot!=8)throw std::runtime_error("unrecovered float layer notification slot");
 auto record=reinterpret_cast<const SessionAttributeRecord*>(r.attributes.table.resolve(attribute));
 if(!record)throw std::runtime_error("stale float layer notification attribute");
 auto e=r.elements.resolve(r.graph,record->ownerHandle);
 if(!e||r.graph.strings.at(e->type)!="DmeFloatLogLayer"||!r.attributes.resolve(r.graph,r.elements,attribute))throw std::runtime_error("invalid float layer notification owner");
}
