#pragma once
// Requires native Graph Attribute/Value declarations. Constructor defaults only.
inline Attribute makeDefaultFloatAttribute(uint32_t nameIndex){Attribute a;a.name=nameIndex;a.type=3;Value value;value.wire.assign(4,0);a.values.push_back(std::move(value));return a;}
inline Attribute makeDefaultFloatArrayAttribute(uint32_t nameIndex){Attribute a;a.name=nameIndex;a.type=17;return a;}
