#pragma once
#include "session_runtime.h"
struct RuntimeFloatLayerState{
 struct Record{uint32_t owner=0xffffffff;bool left=false,right=false;};
 SessionRuntime&runtime;std::map<uint32_t,Record>records;
 explicit RuntimeFloatLayerState(SessionRuntime&r):runtime(r){}
 Record* resolve(uint32_t layer){
  auto e=runtime.elements.resolve(runtime.graph,layer);if(!e||runtime.graph.strings.at(e->type)!="DmeFloatLogLayer")return nullptr;
  auto it=records.find(layer);return it==records.end()?nullptr:&it->second;
 }
 void construct(uint32_t layer){auto e=runtime.elements.resolve(runtime.graph,layer);if(!e||runtime.graph.strings.at(e->type)!="DmeFloatLogLayer"||!records.emplace(layer,Record{}).second)throw std::runtime_error("invalid runtime layer construction");}
 void attach(uint32_t layer,uint32_t log){auto state=resolve(layer);auto e=runtime.elements.resolve(runtime.graph,log);if(!state||!e||runtime.graph.strings.at(e->type)!="DmeFloatLog")throw std::runtime_error("invalid runtime layer owner");state->owner=log;}
 size_t rebindLogLayers(uint32_t log,size_t first,size_t last){
  auto e=runtime.elements.resolve(runtime.graph,log);if(!e||runtime.graph.strings.at(e->type)!="DmeFloatLog")throw std::runtime_error("invalid owner rebind log");
  auto a=runtime.attributes.resolve(runtime.graph,runtime.elements,runtime.attributes.find(log,"layers"));if(!a||a->type!=15||first>last||last>=a->values.size())throw std::runtime_error("invalid owner rebind range");
  size_t count=0;for(size_t i=first;i<=last;++i){const auto&v=a->values[i];if(v.reference>=runtime.graph.elements.size())continue;auto layer=runtime.elements.graphHandles.at(v.reference);if(!resolve(layer))continue;attach(layer,log);++count;}return count;
 }
 uint32_t owner(uint32_t layer){auto state=resolve(layer);return state&&runtime.elements.resolve(runtime.graph,state->owner)?state->owner:0xffffffff;}
};

