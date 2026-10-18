#pragma once
#include "reference_mark_policy.h"
#include <vector>
#include <stdexcept>
struct MarkReferenceEdge{uint32_t destination;uint16_t flags;};
struct MarkReferenceNode{uint16_t marks=0;std::vector<MarkReferenceEdge>edges;};
// Explicit runtime flags/resolved edges; no inference from serialized types.
inline void markReferenceGraph(std::vector<MarkReferenceNode>&nodes,uint32_t root,bool value,uint32_t mode,uint32_t bit){
 uint16_t mask=uint16_t(uint32_t(1)<<(bit&31));std::vector<uint32_t> pending{root};
 while(!pending.empty()){
  uint32_t id=pending.back();pending.pop_back();if(id==0xffffffff)continue;
  auto&node=nodes.at(id);if(bool(node.marks&mask)==value)continue;
  node.marks=value?uint16_t(node.marks|mask):uint16_t(node.marks&~mask);
  for(auto i=node.edges.rbegin();i!=node.edges.rend();++i)if(referenceMarkTraverses(i->flags,mode))pending.push_back(i->destination);
 }
}
