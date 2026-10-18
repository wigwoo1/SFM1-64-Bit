#pragma once
#include "quaternion_transform_matrix.h"
#include <cstring>
// Requires Graph and namedAttribute from the graph/hierarchy headers.
inline RecoveredMatrix3x4 graphLocalTransformMatrix(const Graph&g,uint32_t id,bool applyScale=false) {
    const auto& e=g.elements.at(id);
    if(g.strings.at(e.type)!="DmeTransform") throw std::runtime_error("expected DmeTransform");
    const auto* position=namedAttribute(g,e,"position");
    const auto* orientation=namedAttribute(g,e,"orientation");
    if(!position || position->type!=10 || position->values.size()!=1 || position->values[0].wire.size()!=12)
        throw std::runtime_error("invalid transform position");
    if(!orientation || orientation->type!=13 || orientation->values.size()!=1 || orientation->values[0].wire.size()!=16)
        throw std::runtime_error("invalid transform orientation");
    std::array<float,3> p{};std::array<float,4> q{};
    std::memcpy(p.data(),position->values[0].wire.data(),12);
    std::memcpy(q.data(),orientation->values[0].wire.data(),16);
    auto matrix=recoveredQuaternionTransformMatrix(q,p);
    // Installed CDmeTransform_GetTransform wrapper 10293AEF calls
    // 100775C0, which scales the nine basis entries, retaining translation.
    // Bone pose callers compose scale separately and use the default here.
    const auto*scale=applyScale?namedAttribute(g,e,"scale"):nullptr;
    if(scale&&scale->type==3){if(scale->values.size()!=1||scale->values[0].wire.size()!=4)throw std::runtime_error("invalid transform scale");const float value=wireFloat(scale->values[0]);if(!std::isfinite(value))throw std::runtime_error("nonfinite transform scale");for(unsigned row=0;row<3;++row)for(unsigned column=0;column<3;++column){matrix[row][column]*=value;if(!std::isfinite(matrix[row][column]))throw std::runtime_error("transform scale overflow");}}
    return matrix;
}
