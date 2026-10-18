#pragma once
#include "session_runtime.h"
#include "operator_attribute_registration.h"
// Requires AttributeOperation from the verified channel/expression contracts.
inline size_t registerSessionAttributeOperation(SessionRuntime&r,OperatorAttributeRegistration&registry,const AttributeOperation&operation){
 auto owner=r.elements.graphHandles.at(operation.id);auto identity=r.elements.table.resolve(owner);if(!identity)throw std::runtime_error("retired operation registration");
 auto attribute=[&](const AttributeIdentity&a){auto handle=r.attributes.find(r.elements.graphHandles.at(a.first),a.second);auto value=r.attributes.identity(r.graph,r.elements,handle);if(!value)throw std::runtime_error("missing operation attribute identity");return value;};
 std::vector<uintptr_t> inputs,outputs;for(const auto&a:operation.inputs)inputs.push_back(attribute(a));for(const auto&a:operation.outputs)outputs.push_back(attribute(a));return registry.add(identity,inputs,outputs);
}
