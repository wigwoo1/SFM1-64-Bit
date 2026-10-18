#pragma once
// Prototype incremental execution atop recovered attribute dependencies.
// Native framework phase/owner flags still require integration.
class SessionChangeScheduler {
 std::vector<AttributeOperation> operations;std::vector<uint32_t> order;
 std::map<uint32_t,size_t> indices;
 std::map<AttributeIdentity,std::vector<size_t>> consumers;
public:
 explicit SessionChangeScheduler(std::vector<AttributeOperation> input):operations(std::move(input)){
  for(size_t i=0;i<operations.size();++i){if(!indices.emplace(operations[i].id,i).second)throw std::runtime_error("duplicate operation");for(const auto&attribute:operations[i].inputs)consumers[attribute].push_back(i);}
  auto nodes=attributeDependencies(operations);std::vector<uint32_t> roots;for(uint32_t i=0;i<nodes.size();++i)roots.push_back(i);auto schedule=traverseSchedule(nodes,roots);
  if(schedule.cycle)throw std::runtime_error("incremental cycle policy unresolved");order=std::move(schedule.order);
 }
 template<class Execute> std::vector<uint32_t> step(const std::vector<uint32_t>&requested,const std::vector<AttributeIdentity>&changedInputs,Execute execute){
  std::vector<bool> dirty(operations.size(),false);std::vector<uint32_t> executed;
  auto invalidate=[&](const AttributeIdentity&attribute){auto found=consumers.find(attribute);if(found!=consumers.end())for(auto index:found->second)dirty[index]=true;};
  for(auto id:requested)dirty.at(indices.at(id))=true;for(const auto&attribute:changedInputs)invalidate(attribute);
  for(auto id:order){size_t index=indices.at(id);if(!dirty[index])continue;SessionAttributeChanges changes;
   if(!execute(id,changes))throw std::runtime_error("unsupported scheduled operation");executed.push_back(id);
   for(const auto&attribute:changes.attributes)invalidate(attribute);
  }return executed;
 }
};
