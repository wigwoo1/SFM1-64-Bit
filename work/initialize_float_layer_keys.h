#pragma once
#include "session_runtime.h"
#include <cstring>
// Explicit graph key edit; native key-edit notifications/undo remain separate.
inline bool initializeEmptyFloatLayerKeys(SessionRuntime&r,uint32_t layer,const std::vector<std::pair<int32_t,float>>&keys){
 auto e=r.elements.resolve(r.graph,layer);if(!e||r.graph.strings.at(e->type)!="DmeFloatLogLayer"||keys.empty())return false;
 auto get=[&](const char*name){return r.attributes.resolve(r.graph,r.elements,r.attributes.find(layer,name));};
 auto times=get("times"),values=get("values"),curves=get("curvetypes"),compressed=get("compressed");
 if(!times||!values||!curves||!compressed||times->type!=21||values->type!=17||curves->type!=16||compressed->type!=6||!times->values.empty()||!values->values.empty()||!curves->values.empty()||compressed->values.size()!=1||!compressed->values[0].wire.empty())return false;
 std::vector<Value>newTimes,newValues;newTimes.reserve(keys.size());newValues.reserve(keys.size());
 for(size_t i=0;i<keys.size();++i){if(i&&keys[i].first<=keys[i-1].first)return false;Value time,value;time.wire.resize(4);value.wire.resize(4);setWireU32(time,static_cast<uint32_t>(keys[i].first));uint32_t bits;std::memcpy(&bits,&keys[i].second,4);setWireU32(value,bits);newTimes.push_back(std::move(time));newValues.push_back(std::move(value));}
 times->values.swap(newTimes);values->values.swap(newValues);return true;
}
