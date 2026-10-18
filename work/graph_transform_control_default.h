#pragma once
#include "clip_hierarchy.h"
#include <array>
// Getter104CC000: absent attribute returns shared zero vector10F4ED24.
// Caller supplies an already selected control; subclass/selection order remains separate.
inline std::array<float,3> graphSelectedControlDefaultPositionCandidate(const Graph&g,uint32_t control,const std::array<float,3>&globalZero){
 if(control==0xffffffffu)return globalZero;
 const auto*a=namedAttribute(g,g.elements.at(control),"defaultPosition");
 if(!a)return globalZero;
 if(a->type!=10||a->values.size()!=1||a->values[0].wire.size()!=12)throw std::runtime_error("invalid control defaultPosition");
 std::array<float,3>out{};std::memcpy(out.data(),a->values[0].wire.data(),12);return out;
}
