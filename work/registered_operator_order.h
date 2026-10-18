#pragma once
#include "operator_attribute_registration.h"
#include "operator_dependency_order.h"
// Native recursive call receives attribute association vector (+4), so each
// operator edge expands its ordered associated operators, preserving duplicates.
inline OperatorDependencyOrder orderRegisteredOperators(const OperatorAttributeRegistration&registration,const std::vector<size_t>&roots){
 std::vector<OperatorDependencyNode> nodes;nodes.reserve(registration.operators.size());
 for(const auto&op:registration.operators){OperatorDependencyNode node;node.identity=op.identity;
  for(auto index:op.attributeEdges){if(index>=registration.attributes.size())throw std::runtime_error("attribute edge index out of range");const auto&associated=registration.attributes[index].associatedOperators;node.edges.insert(node.edges.end(),associated.begin(),associated.end());}
  nodes.push_back(std::move(node));
 }return orderOperatorDependencies(nodes,roots);
}
