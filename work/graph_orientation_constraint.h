#pragma once
#include "graph_transform_hierarchy.h"
#include "quaternion_product.h"
#include "quaternion_log_average.h"
struct RecoveredOrientationConstraintAverage {
    std::array<float,4> orientation;
    float influence;
};
inline RecoveredOrientationConstraintAverage graphOrientationConstraintAverage(
    const Graph&g,GraphTransformHierarchy&hierarchy,uint32_t constraint,
    const std::array<float,4>&initialOutput) {
    if(&hierarchy.graph!=&g)throw std::runtime_error("orientation hierarchy mismatch");
    const auto& e=g.elements.at(constraint);
    if(g.strings.at(e.type)!="DmeRigOrientConstraintOperator")throw std::runtime_error("expected orientation constraint");
    auto targetIds=hierarchyRefs(g,e,"targets",15);
    std::vector<std::array<float,4>> orientations;std::vector<float>weights;
    float total=0;
    for(auto id:targetIds){const auto& target=g.elements.at(id);
        if(g.strings.at(target.type)!="DmeConstraintTarget")throw std::runtime_error("invalid orientation target");
        auto dags=hierarchyRefs(g,target,"target",1);if(dags.empty())continue;
        const auto* weight=namedAttribute(g,target,"targetWeight");
        const auto* offset=namedAttribute(g,target,"oOffset");
        if(!weight||weight->type!=3||weight->values.size()!=1||weight->values[0].wire.size()!=4)
            throw std::runtime_error("invalid orientation weight");
        if(!offset||offset->type!=13||offset->values.size()!=1||offset->values[0].wire.size()!=16)
            throw std::runtime_error("invalid orientation offset");
        float w;std::array<float,4> q{};
        std::memcpy(&w,weight->values[0].wire.data(),4);std::memcpy(q.data(),offset->values[0].wire.data(),16);
        orientations.push_back(multiplyRecoveredAlignedQuaternions(hierarchy.worldOrientation(dags[0]),q));
        weights.push_back(w);total=w+total;
    }
    if(targetIds.size()==1 && orientations.empty())return {initialOutput,0};
    return {recoveredQuaternionLogAverageCandidate(orientations,weights),total>1.0f?1.0f:total};
}
