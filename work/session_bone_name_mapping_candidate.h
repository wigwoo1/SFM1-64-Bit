#pragma once
#include "model_skinning_geometry.h"
#include <map>
// Saved labels supply names, but do not prove parent topology compatibility.
// Caller must explicitly opt in until native remapping parity is verified.
inline std::vector<uint32_t>sessionBoneNameMappingCandidate(const Graph&graph,const Attribute&attribute,const std::vector<GeometryBoneCandidate>&bones){
 std::map<std::string,uint32_t>saved;for(size_t n=0;n<attribute.values.size();++n){auto id=attribute.values[n].reference;auto&element=graph.elements.at(id);auto label=graph.strings.at(element.name);const auto suffix=label.rfind(" (");if(suffix==std::string::npos||suffix+3>label.size()||label.back()!=')'||suffix+2==label.size()-1)throw std::runtime_error("saved bone name schema");auto name=label.substr(suffix+2,label.size()-suffix-3);if(!saved.emplace(name,id).second)throw std::runtime_error("duplicate saved bone name");}
 std::vector<uint32_t>mapping;std::map<std::string,bool>installed;for(auto&bone:bones){if(!installed.emplace(bone.name,true).second)throw std::runtime_error("duplicate installed bone name");auto found=saved.find(bone.name);if(found==saved.end())throw std::runtime_error("saved bone name missing: "+bone.name);mapping.push_back(found->second);}if(mapping.size()!=saved.size())throw std::runtime_error("bone name set mismatch");return mapping;
}
