#pragma once
#include "session_material_overrides_candidate.h"
#include "vmt_keyvalues_candidate.h"
#include <sstream>
#include <iomanip>
#include <locale>
#include <cmath>
#include <limits>
inline std::string sessionMaterialParameterTextCandidate(const SessionMaterialParameterCandidate&parameter){
 if(parameter.values.size()!=1||parameter.type>=15)throw std::runtime_error("array material override unsupported");
 const auto&value=parameter.values.front();if(parameter.type==5)return value.text;
 if(parameter.type==2){if(value.wire.size()!=4)throw std::runtime_error("integer material override wire");return std::to_string(wireInt(value));}
 if(parameter.type==4){if(value.wire.size()!=1||uint8_t(value.wire[0])>1)throw std::runtime_error("boolean material override wire");return value.wire[0]?"1":"0";}
 size_t components=parameter.type==3?1:parameter.type==9?2:parameter.type==10?3:parameter.type==11?4:parameter.type==14?16:0;
 if(!components||value.wire.size()!=components*4)throw std::runtime_error("material override type unsupported");
 std::ostringstream out;out.imbue(std::locale::classic());out<<std::setprecision(std::numeric_limits<float>::max_digits10);if(components>1)out<<'[';
 for(size_t n=0;n<components;++n){float number=wireFloat(value,n);if(!std::isfinite(number))throw std::runtime_error("nonfinite material override");if(n)out<<' ';out<<number;}
 if(components>1)out<<']';return out.str();
}
// Candidate precedence: model-scoped parameters replace matching root VMT keys.
// Source shader conversion and native precedence still require comparison.
inline VmtNodeCandidate applySessionMaterialOverridesCandidate(VmtNodeCandidate syntax,const SessionMaterialOverrideCandidate&record){
 for(const auto&parameter:record.parameters){auto value=sessionMaterialParameterTextCandidate(parameter.second);size_t matches=0;for(auto&node:syntax.children)if(vmtAsciiLowerCandidate(node.key)==parameter.first){if(node.block||++matches>1)throw std::runtime_error("ambiguous VMT override target");node.value=value;}if(!matches)syntax.children.push_back({parameter.first,std::move(value),{},false});}return syntax;
}
