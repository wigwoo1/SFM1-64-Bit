#pragma once
#include "mdl_flex_metadata.h"
#include "global_flex_registry.h"
#include <cmath>
// Extract named current operator values into model-local controller order.
// Does not reinterpret historic global flexWeights indices or evaluate channels.
inline std::vector<float>readSessionLocalFlexControllersCandidate(const Graph&graph,uint32_t model,const std::vector<MdlFlexControllerMetadata>&metadata){
 if(graph.strings.at(graph.elements.at(model).type)!="DmeGameModel")throw std::runtime_error("flex model type");
 auto attribute=[&](const Element&element,const std::string&name)->const Attribute*{const Attribute*found=nullptr;for(const auto&a:element.attributes)if(graph.strings.at(a.name)==name){if(found)throw std::runtime_error("duplicate flex attribute");found=&a;}return found;};
 std::vector<float>values(metadata.size());std::vector<bool>bound(metadata.size());
 for(const auto&element:graph.elements){if(graph.strings.at(element.type)!="DmeGlobalFlexControllerOperator")continue;auto ref=attribute(element,"gameModel");if(!ref)continue;if(ref->type!=1||ref->values.size()!=1)throw std::runtime_error("invalid flex model reference");if(ref->values[0].reference!=model)continue;
  auto source=attribute(element,"flexWeight");if(!source||source->type!=3||source->values.size()!=1||source->values[0].wire.size()!=4)throw std::runtime_error("invalid local flex source");float value;std::memcpy(&value,source->values[0].wire.data(),4);if(!std::isfinite(value))throw std::runtime_error("nonfinite local flex source");
  const auto&name=graph.strings.at(element.name);bool matched=false;for(size_t i=0;i<metadata.size();++i){if(metadata[i].localIndex!=i)throw std::runtime_error("local controller order mismatch");if(!recoveredFlexNameEqual(metadata[i].name,name))continue;if(bound[i]&&values[i]!=value)throw std::runtime_error("conflicting named flex sources");values[i]=value;bound[i]=true;matched=true;}
  if(!matched)throw std::runtime_error("unknown named flex source");
 }
 for(bool present:bound)if(!present)throw std::runtime_error("unbound local flex controller");return values;
}
