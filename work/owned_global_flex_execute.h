#pragma once
#include "global_flex_execute.h"
#include <stdexcept>
#include <cstring>
// Explicit bindings use graph indices, not native attribute handles. Caller owns
// mapping/activation/order. Structural graph edits invalidate these bindings.
struct OwnedGlobalFlexExecuteBindingCandidate{uint32_t operatorElement,sourceAttribute,modelElement,destinationAttribute,weightIndex;};
struct OwnedGlobalFlexExecuteResultCandidate{RecoveredFlexExecuteDispatchCandidate dispatch;bool changed=false;};
template<class Projection>
OwnedGlobalFlexExecuteResultCandidate executeOwnedGlobalFlexCandidate(Projection&projection,const OwnedGlobalFlexExecuteBindingCandidate&binding,int phase){
 struct Destination{uint16_t flags;};Destination destination{};bool changed=false;
 auto dispatch=recoveredFlexExecuteCandidate(RecoveredFlexBindingCache{binding.modelElement,binding.weightIndex},[&](uint32_t model)->Destination*{
 if(model>=projection.elementHandles.size()||!projection.scene.resolve(projection.elementHandles[model]))return nullptr;
 const auto&e=projection.graph.elements.at(model);if(binding.destinationAttribute>=e.attributes.size())return nullptr;
 const auto&a=e.attributes[binding.destinationAttribute];if(a.type!=17||projection.graph.strings.at(a.name)!="flexWeights")throw std::runtime_error("unsupported flex destination");destination.flags=projection.typedAttributeFlags.at(model).at(binding.destinationAttribute);return &destination;
 },[&](){
 if(binding.operatorElement>=projection.elementHandles.size()||!projection.scene.resolve(projection.elementHandles[binding.operatorElement]))throw std::runtime_error("missing flex source owner");
 const auto&e=projection.graph.elements.at(binding.operatorElement);if(projection.graph.strings.at(e.type)!="DmeGlobalFlexControllerOperator")throw std::runtime_error("wrong flex operator class");
 const auto&a=e.attributes.at(binding.sourceAttribute);if(projection.graph.strings.at(a.name)!="flexWeight"||a.type!=3||a.values.size()!=1||a.values[0].wire.size()!=4)throw std::runtime_error("unsupported flex source");uint32_t bits;std::memcpy(&bits,a.values[0].wire.data(),4);return RecoveredFlexSourceValueCandidate{projection.typedAttributeFlags.at(binding.operatorElement).at(binding.sourceAttribute),bits};
 },[&](Destination*,uint32_t index,uint32_t sourceType,uint32_t bits){if(sourceType!=3)return;int32_t signedIndex;std::memcpy(&signedIndex,&index,4);changed=projection.setFloatArrayElement(binding.modelElement,"flexWeights",signedIndex,bits,phase);
 },[](Destination*,uint32_t,uint32_t){throw std::runtime_error("unsupported scalar flex destination");});
 return {dispatch,changed};
}
