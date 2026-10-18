#pragma once
#include "float_attribute_defaults.h"
// Requires Graph declarations. Defaults verified through native type allocators.
inline Attribute makeDefaultLogArrayAttribute(uint32_t nameIndex,uint8_t type){if(type!=15&&type!=16&&type!=17&&type!=21)throw std::runtime_error("unverified log array default type");Attribute a;a.name=nameIndex;a.type=type;return a;}
inline Attribute makeDefaultLogReferenceAttribute(uint32_t nameIndex){Attribute a;a.name=nameIndex;a.type=1;Value value;value.reference=0xffffffff;a.values.push_back(std::move(value));return a;}
