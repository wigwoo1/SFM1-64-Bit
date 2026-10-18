#pragma once
#include "clip_hierarchy.h"
inline bool sessionModelVisibilityCandidate(const Graph&g,uint32_t model){auto&e=g.elements.at(model);if(g.strings.at(e.type)!="DmeGameModel")throw std::runtime_error("visibility requires model");auto*a=namedAttribute(g,e,"visible");if(!a||a->type!=4||a->values.size()!=1||a->values[0].wire.size()!=1)throw std::runtime_error("model visibility schema");auto value=static_cast<unsigned char>(a->values[0].wire[0]);if(value>1)throw std::runtime_error("noncanonical model visibility");return value!=0;}
