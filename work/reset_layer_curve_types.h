#pragma once
#include "runtime_float_layer_state.h"
#include <optional>
// Graph effect of 0x1054C980; caller resolves owner curveinfo default type.
// Native integer-array clear/append mutation notifications remain separate.
inline bool resetRuntimeFloatLayerCurveTypes(RuntimeFloatLayerState&state,uint32_t layer,bool suppressed,std::optional<int32_t>defaultType){
 if(!state.resolve(layer))return false;if(suppressed)return true;
 auto&r=state.runtime;auto times=r.attributes.resolve(r.graph,r.elements,r.attributes.find(layer,"times"));auto curves=r.attributes.resolve(r.graph,r.elements,r.attributes.find(layer,"curvetypes"));
 if(!times||!curves||times->type!=21||curves->type!=16)return false;
 std::vector<Value>replacement;
 if(defaultType){replacement.reserve(times->values.size());for(size_t i=0;i<times->values.size();++i){Value v;v.wire.resize(4);setWireU32(v,static_cast<uint32_t>(*defaultType));replacement.push_back(std::move(v));}}
 curves->values.swap(replacement);return true;
}
