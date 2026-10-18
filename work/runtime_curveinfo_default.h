#pragma once
#include "reset_layer_curve_types.h"
inline std::optional<int32_t> runtimeLayerDefaultCurveType(RuntimeFloatLayerState&state,uint32_t layer){
 auto&r=state.runtime;auto owner=state.owner(layer);if(owner==0xffffffff)return std::nullopt;
 auto a=r.attributes.resolve(r.graph,r.elements,r.attributes.find(owner,"curveinfo"));if(!a||a->type!=1||a->values.size()!=1)throw std::runtime_error("invalid owner curveinfo");
 auto ref=a->values[0].reference;if(ref==0xffffffff)return std::nullopt;if(ref>=r.graph.elements.size())throw std::runtime_error("curveinfo requires external resolver");
 auto h=r.elements.graphHandles.at(ref);auto value=r.attributes.resolve(r.graph,r.elements,r.attributes.find(h,"defaultCurveType"));if(!value||value->type!=2||value->values.size()!=1)throw std::runtime_error("invalid defaultCurveType schema");return wireInt(value->values[0]);
}
inline bool refreshRuntimeLayerCurveTypes(RuntimeFloatLayerState&state,uint32_t layer,bool suppressed){
 if(suppressed)return state.resolve(layer)!=nullptr;
 return resetRuntimeFloatLayerCurveTypes(state,layer,false,runtimeLayerDefaultCurveType(state,layer));
}
