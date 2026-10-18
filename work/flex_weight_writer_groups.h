#pragma once
#include "owned_global_flex_execute.h"
#include <map>
#include <tuple>
#include <vector>
struct FlexWeightWriterGroupsCandidate{size_t slots=0,sharedSlots=0,sharedWriters=0;};
// Current-value equivalence only. Shared slots still execute serially; no
// native callback order or parallel mutation safety is inferred from this.
template<class Projection>
FlexWeightWriterGroupsCandidate validateFlexWeightWriterGroupsCandidate(const Projection&projection,const std::vector<OwnedGlobalFlexExecuteBindingCandidate>&bindings){
 using Key=std::tuple<uint32_t,uint32_t,uint32_t>;
 std::map<Key,std::vector<const OwnedGlobalFlexExecuteBindingCandidate*>> groups;
 for(const auto&b:bindings)groups[{b.modelElement,b.destinationAttribute,b.weightIndex}].push_back(&b);
 FlexWeightWriterGroupsCandidate result{groups.size(),0,0};
 for(const auto&group:groups){if(group.second.size()<2)continue;++result.sharedSlots;result.sharedWriters+=group.second.size();bool first=true;uint32_t expected=0;
 for(const auto*b:group.second){const auto&a=projection.graph.elements.at(b->operatorElement).attributes.at(b->sourceAttribute);if(a.type!=3||a.values.size()!=1||a.values[0].wire.size()!=4)throw std::runtime_error("invalid shared flex source");uint32_t bits;std::memcpy(&bits,a.values[0].wire.data(),4);
 if((bits&0x7f800000u)==0x7f800000u)throw std::runtime_error("nonfinite shared flex writers require ordering policy");
 if(first){expected=bits;first=false;}else if(bits!=expected)throw std::runtime_error("conflicting shared flex writers require ordering policy");
 }}return result;
}
