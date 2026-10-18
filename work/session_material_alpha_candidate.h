#pragma once
#include "session_material_overrides_candidate.h"
#include <cmath>
struct SessionMaterialAlphaCandidate {std::string material;float alpha;};
struct SessionMaterialDynamicCandidate {std::vector<SessionMaterialOverrideCandidate>resources;std::vector<SessionMaterialAlphaCandidate>alpha;};
// Alpha is a per-draw candidate parameter, not a texture resource identity.
// Shader application and native blend/depth semantics remain unfinished.
inline SessionMaterialDynamicCandidate partitionSessionMaterialAlphaCandidate(std::vector<SessionMaterialOverrideCandidate>records){
 SessionMaterialDynamicCandidate result;
 for(auto&record:records){auto found=record.parameters.find("$alpha");if(found==record.parameters.end())continue;const auto&parameter=found->second;if(parameter.type!=3||parameter.values.size()!=1||parameter.values[0].wire.size()!=4)throw std::runtime_error("material alpha schema");float alpha=wireFloat(parameter.values[0]);if(!std::isfinite(alpha)||alpha<0||alpha>1)throw std::runtime_error("material alpha range");result.alpha.push_back({record.material,alpha});record.parameters.erase(found);}
 result.resources=std::move(records);return result;
}
