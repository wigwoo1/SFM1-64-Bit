#pragma once
#include <cstdint>
#include <optional>
#include <stdexcept>
struct SessionModelIlluminationDagCandidate {
 bool attributePresent=false,firstChildAttempted=false;
 std::optional<uint32_t>dag;
};
// IFM10243967..102439B5: absent attribute enters a separate renderer fallback,
// while a present unresolved/non-DAG reference tries child0 only. Type ancestry
// resolution is a caller-supplied contract; do not guess it from a transform
// attribute. This reader does not implement the absent-attribute renderer path.
template<class IsDag>
SessionModelIlluminationDagCandidate sessionModelIlluminationDagCandidate(
 const Graph&graph,uint32_t model,IsDag isDag){
 const auto&e=graph.elements.at(model);if(graph.strings.at(e.type)!="DmeGameModel")throw std::runtime_error("illumination binding requires DmeGameModel");
 SessionModelIlluminationDagCandidate result;const auto*a=namedAttribute(graph,e,"illumPositionDag");if(!a)return result;result.attributePresent=true;
 if(a->type!=1||a->values.size()!=1)throw std::runtime_error("invalid illumination DAG reference schema");
 auto resolve=[&](uint32_t ref)->std::optional<uint32_t>{if(ref==UINT32_MAX)return {};if(ref>=graph.elements.size())throw std::runtime_error("unresolved illumination DAG reference");if(!isDag(ref))return {};return ref;};
 result.dag=resolve(a->values.front().reference);if(result.dag)return result;
 const auto*children=namedAttribute(graph,e,"children");if(!children)return result;if(children->type!=15)throw std::runtime_error("invalid illumination child array");if(children->values.empty())return result;
 result.firstChildAttempted=true;result.dag=resolve(children->values.front().reference);return result;
}
