#pragma once
#include "runtime_float_layer_state.h"
#include "session_reference_propagation.h"
// Supported float-log portion of native post-restore 0x1035F010.
// Attribute/cross-file repair and slot +0x18 remain explicit caller hooks.
template<class Repair,class Restored,class Suppressed>
size_t postRestoreFloatLog(SessionRuntime&r,MutationCoordinator&m,RuntimeFloatLayerState&state,uint32_t log,uint32_t&frameworkDirty,Repair repair,Restored restored,Suppressed suppressed){
 auto e=r.elements.resolve(r.graph,log);if(&state.runtime!=&r||!e||r.graph.strings.at(e->type)!="DmeFloatLog"||!m.owners.count(log))throw std::runtime_error("unsupported float log post-restore");
 repair(log);restored(log);
 auto a=r.attributes.resolve(r.graph,r.elements,r.attributes.find(log,"layers"));if(!a||a->type!=15)throw std::runtime_error("post-restore layers missing");
 auto rebound=a->values.empty()?0:state.rebindLogLayers(log,0,a->values.size()-1);
 auto&owner=m.owners.at(log);owner.state&=0xfc;for(auto&flag:owner.attributeFlags)flag.second&=0xff7f;
 propagateSessionReferences(r,m,log,3,suppressed);frameworkDirty|=0x1000;return rebound;
}
