#pragma once
#include "clip_hierarchy.h"
inline uint32_t sessionModelBodyCandidate(const Graph&g,uint32_t id){auto&e=g.elements.at(id);if(g.strings.at(e.type)!="DmeGameModel")throw std::runtime_error("body requires model");auto*a=namedAttribute(g,e,"body");if(!a||a->type!=2||a->values.size()!=1||a->values[0].wire.size()!=4)throw std::runtime_error("model body schema");auto body=wireInt(a->values[0]);if(body<0)throw std::runtime_error("negative model body");return uint32_t(body);}
