#pragma once
#include "owned_global_flex_execute.h"
#include "model_flex_lookup.h"
#include <vector>
#include <map>
struct OwnedGlobalFlexBindingPlanCandidate{
 std::vector<OwnedGlobalFlexExecuteBindingCandidate> bindings;
 size_t missingMetadata=0,missingController=0,outOfRange=0;
};
// Explicit reconstruction policy: models and their operators in serialized order.
// Registry and metadata runtime indices must belong to the same cache lifetime.
// Does not modify graph weights or silently apply required native array growth.
template<class Projection,class ResolveMetadata>
OwnedGlobalFlexBindingPlanCandidate discoverOwnedGlobalFlexBindingsCandidate(Projection&projection,RecoveredGlobalFlexRegistryCandidate&registry,ResolveMetadata resolveMetadata){
 OwnedGlobalFlexBindingPlanCandidate plan;auto&g=projection.graph;
 auto find=[&](const auto&e,const std::string&name){for(uint32_t j=0;j<e.attributes.size();++j)if(g.strings.at(e.attributes[j].name)==name)return j;return 0xffffffffu;};
 // Index once; vectors retain serialized operator order within each model.
 std::map<uint32_t,std::vector<uint32_t>> operatorsByModel;
 for(uint32_t op=0;op<g.elements.size();++op){const auto&o=g.elements[op];if(g.strings.at(o.type)!="DmeGlobalFlexControllerOperator")continue;auto attribute=find(o,"gameModel");if(attribute==0xffffffffu)continue;const auto&ref=o.attributes[attribute];if(ref.type==1&&ref.values.size()==1)operatorsByModel[ref.values[0].reference].push_back(op);}
 for(uint32_t model=0;model<g.elements.size();++model){auto&e=g.elements[model];if(g.strings.at(e.type)!="DmeGameModel")continue;auto dest=find(e,"flexWeights");if(dest==0xffffffffu||e.attributes[dest].type!=17)continue;
 auto*records=resolveMetadata(model);std::vector<uint32_t>weights;for(const auto&v:e.attributes[dest].values){if(v.wire.size()!=4)throw std::runtime_error("float array wire size");uint32_t bits;std::memcpy(&bits,v.wire.data(),4);weights.push_back(bits);}const size_t savedCount=weights.size();
 for(uint32_t op:operatorsByModel[model]){const auto&o=g.elements[op];auto source=find(o,"flexWeight");if(source==0xffffffffu||o.attributes[source].type!=3||o.attributes[source].values.size()!=1||o.attributes[source].values[0].wire.size()!=4)throw std::runtime_error("invalid flex source");
 if(!records){++plan.missingMetadata;continue;}auto index=recoveredModelFlexLookupCandidate(records,g.strings.at(o.name),weights,&registry);if(index==0xffffffffu){++plan.missingController;continue;}
 if(index>=savedCount)++plan.outOfRange;plan.bindings.push_back({op,source,model,dest,index});
 }}return plan;
}
