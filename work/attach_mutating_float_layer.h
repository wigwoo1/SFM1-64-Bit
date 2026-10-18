#pragma once
#include "append_runtime_reference.h"
#include "session_reference_propagation.h"
#include "runtime_float_layer_state.h"
// Caller initializes recovered schema metadata; transient owner/file/undo
// lifecycle and native framework dirty flags remain separate.
template<class Notify,class Suppressed>
bool attachMutatingFloatLayer(SessionRuntime&r,MutationCoordinator&m,uint32_t log,uint32_t layer,bool preparationSuppressed,Notify notify,Suppressed suppressed,RuntimeFloatLayerState*layerState=nullptr){
 auto owner=r.elements.resolve(r.graph,log);auto target=r.elements.resolve(r.graph,layer);
 if(!owner||!target||r.graph.strings.at(owner->type)!="DmeFloatLog"||r.graph.strings.at(target->type)!="DmeFloatLogLayer")return false;
 if(layerState&&(&layerState->runtime!=&r||!layerState->resolve(layer)))return false;
 return appendRuntimeReference(r,m,log,"layers",layer,preparationSuppressed,[&](unsigned phase,uint32_t attribute,size_t first,size_t last){
  if(layerState&&phase==1)layerState->rebindLogLayers(log,first,last);
  notify(phase,attribute,first,last);
 },[&](uint32_t handle,bool refs){propagateSessionReferences(r,m,handle,refs?3:2,suppressed);});
}
