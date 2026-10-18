#pragma once
#include "session_mutation_dispatch.h"
#include "runtime_float_layer_state.h"
// Native float-log marked callback slot +0x0C at 0x10516F60.
// Other class callbacks stay explicit; no load-time queue membership is guessed.
template<class Other> size_t dispatchMarkedFloatLogOwners(SessionMutationDispatch&dispatch,RuntimeFloatLayerState&state,Other other){
 if(&state.runtime!=&dispatch.runtime)throw std::runtime_error("layer state belongs to different runtime");
 std::map<MutationOwner*,uint32_t>handles;for(auto&entry:dispatch.mutation.owners)handles.emplace(&entry.second,entry.first);size_t count=0;
 dispatch.processMarked([&](MutationOwner*metadata){auto handle=handles.at(metadata);auto e=dispatch.runtime.elements.resolve(dispatch.runtime.graph,handle);
  if(dispatch.runtime.graph.strings.at(e->type)!="DmeFloatLog"){other(handle);return;}
  auto a=dispatch.runtime.attributes.resolve(dispatch.runtime.graph,dispatch.runtime.elements,dispatch.runtime.attributes.find(handle,"layers"));if(!a||a->type!=15)throw std::runtime_error("marked float log schema missing");
  if(!a->values.empty())count+=state.rebindLogLayers(handle,0,a->values.size()-1);
 });return count;
}
