#pragma once
#include "mdl_flex_rules_candidate.h"
#include <cmath>
// Explicit direct-controller rule subset. Inputs are already model-local;
// saved session global arrays must be translated before calling this routine.
inline std::vector<float>evaluateDirectMdlFlexRulesCandidate(const std::vector<MdlFlexRuleCandidate>&rules,size_t descriptorCount,const std::vector<float>&controllers){
 std::vector<float>values(descriptorCount);std::vector<uint32_t>sources(descriptorCount,UINT32_MAX);
 for(const auto&r:rules){if(r.target>=descriptorCount||r.operations.size()!=1||r.operations[0].opcode!=2)throw std::runtime_error("unsupported direct flex rule");auto index=r.operations[0].operandBits;if(index>=controllers.size()||!std::isfinite(controllers[index]))throw std::runtime_error("invalid direct flex controller");if(sources[r.target]!=UINT32_MAX&&sources[r.target]!=index)throw std::runtime_error("conflicting direct flex rules");sources[r.target]=index;values[r.target]=controllers[index];}
 return values;
}
