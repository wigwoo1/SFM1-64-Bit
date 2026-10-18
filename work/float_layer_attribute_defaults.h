#pragma once
#include "log_attribute_defaults.h"
inline std::vector<Attribute> makeFloatLayerAttributeDefaults(Graph&g){
 std::vector<Attribute> attributes;
 attributes.push_back(makeDefaultLogArrayAttribute(internString(g,"times"),21));
 attributes.push_back(makeDefaultLogArrayAttribute(internString(g,"curvetypes"),16));
 attributes.push_back(makeDefaultFloatArrayAttribute(internString(g,"values")));
 Attribute compressed;compressed.name=internString(g,"compressed");compressed.type=6;compressed.values.emplace_back();attributes.push_back(std::move(compressed));return attributes;
}
