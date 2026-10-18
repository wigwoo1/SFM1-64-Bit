#pragma once
#include "clip_hierarchy.h"
inline uint32_t sessionModelSkinCandidate(const Graph&g,uint32_t id){auto&e=g.elements.at(id);if(g.strings.at(e.type)!="DmeGameModel")throw std::runtime_error("skin requires model");auto*a=namedAttribute(g,e,"skin");if(!a||a->type!=2||a->values.size()!=1||a->values[0].wire.size()!=4)throw std::runtime_error("model skin schema");auto skin=wireInt(a->values[0]);if(skin<0)throw std::runtime_error("negative model skin");return uint32_t(skin);}
