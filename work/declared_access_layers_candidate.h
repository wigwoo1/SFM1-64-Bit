#pragma once
#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <stdexcept>
// Requires AttributeOperation and AttributeIdentity. Caller supplies a complete
// serial order and complete access sets; hidden state/aliases are not covered.
inline std::vector<std::vector<uint32_t>> declaredAccessLayersCandidate(
 const std::vector<AttributeOperation>&operations,const std::vector<uint32_t>&serialOrder){
 if(serialOrder.size()!=operations.size())throw std::runtime_error("incomplete access schedule");
 std::map<uint32_t,size_t>indices;for(size_t i=0;i<operations.size();++i)
  if(!indices.emplace(operations[i].id,i).second)throw std::runtime_error("duplicate access operation");
 std::set<uint32_t>seen;std::map<AttributeIdentity,size_t>writers,readers;
 std::vector<std::vector<uint32_t>>layers;
 for(auto id:serialOrder){auto found=indices.find(id);if(found==indices.end()||!seen.insert(id).second)throw std::runtime_error("invalid access schedule");
  const auto&operation=operations[found->second];size_t layer=0;
  auto after=[&](const auto&table,const auto&attribute){auto prior=table.find(attribute);if(prior!=table.end())layer=std::max(layer,prior->second+1);};
  for(const auto&attribute:operation.inputs)after(writers,attribute);
  for(const auto&attribute:operation.outputs){after(writers,attribute);after(readers,attribute);}
  if(layers.size()<=layer)layers.resize(layer+1);layers[layer].push_back(id);
  std::set<AttributeIdentity>outputs(operation.outputs.begin(),operation.outputs.end());
  for(const auto&attribute:outputs){writers[attribute]=layer;readers.erase(attribute);}
  for(const auto&attribute:operation.inputs)if(!outputs.count(attribute)){
   auto inserted=readers.emplace(attribute,layer);if(!inserted.second)inserted.first->second=std::max(inserted.first->second,layer);
  }
 }return layers;
}
