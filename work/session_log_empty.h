#pragma once
// Native 0x10516A80 tests every valid layer's key count. Invalid handles
// and wrong layer types contribute no keys; selection does not affect emptiness.
inline bool sessionFloatLogEmpty(const Graph&graph,const std::vector<uint32_t>&layers){
 for(uint32_t id:layers){
  if(id==0xffffffff||id>=graph.elements.size())continue;
  const auto&layer=graph.elements[id];
  if(graph.strings.at(layer.type)!="DmeFloatLogLayer")continue;
  const auto*times=namedAttribute(graph,layer,"times");
  if(!times||times->type!=21)throw std::runtime_error("invalid float layer times");
  if(!times->values.empty())return false;
 }
 return true;
}
