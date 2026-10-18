#pragma once
// Requires Graph and channel/scheduler declarations.
struct AttributeOperation {uint32_t id;std::vector<AttributeIdentity> inputs,outputs;};
inline AttributeOperation expressionOperation(const Graph&graph,uint32_t id){
 const auto&element=graph.elements.at(id);if(graph.strings.at(element.type)!="DmeExpressionOperator")throw std::runtime_error("expected expression operator");
 AttributeOperation operation{id,{}, {}};
 for(const auto&attribute:element.attributes)if(attribute.type==3){
  if(attribute.values.size()!=1)throw std::runtime_error("invalid expression scalar");
  AttributeIdentity identity{id,graph.strings.at(attribute.name)};
  if(identity.second=="result")operation.outputs.push_back(identity);else operation.inputs.push_back(identity);
 }
 if(operation.outputs.size()!=1)throw std::runtime_error("missing expression result output");return operation;
}
inline AttributeOperation channelOperation(uint32_t id,const ChannelBinding&binding){
 AttributeOperation operation{id,{}, {}};
 if(binding.mode!=0&&binding.mode!=3&&binding.source.present())operation.inputs.push_back({binding.source.element,binding.source.attribute});
 if(binding.mode!=0&&binding.destination.present()){
  AttributeIdentity output{binding.destination.element,binding.destination.attribute};
  bool selfCopy=(binding.mode==1||binding.mode==2)&&binding.source.present()&&output==AttributeIdentity{binding.source.element,binding.source.attribute};
  if(!selfCopy)operation.outputs.push_back(output);
 }return operation;
}
inline std::vector<ScheduleNode> attributeDependencies(const std::vector<AttributeOperation>&operations){
 std::map<AttributeIdentity,std::vector<uint32_t>> consumers;
 for(size_t i=0;i<operations.size();++i)for(const auto&input:operations[i].inputs)consumers[input].push_back(static_cast<uint32_t>(i));
 std::vector<ScheduleNode> nodes;
 for(const auto&operation:operations){ScheduleNode node{operation.id,{}};
  for(const auto&output:operation.outputs){auto found=consumers.find(output);if(found!=consumers.end())node.edgeGroups.push_back(found->second);}nodes.push_back(std::move(node));
 }return nodes;
}
