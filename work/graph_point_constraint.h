#pragma once
#include "graph_transform_hierarchy.h"
#include "point_constraint_accumulator.h"
inline RecoveredPointConstraintAverage graphPointConstraintAverage(
    const Graph&g,GraphTransformHierarchy&hierarchy,uint32_t constraint,
    const std::array<float,3>&initializedGlobalVector) {
    if(&hierarchy.graph!=&g)throw std::runtime_error("constraint hierarchy mismatch");
    const auto& e=g.elements.at(constraint);
    const auto&type=g.strings.at(e.type);
    if(type!="DmeRigPointConstraintOperator"&&type!="DmeRigIKConstraintOperator")throw std::runtime_error("expected point-target constraint");
    std::vector<RecoveredPointConstraintTarget> targets;
    for(auto id:hierarchyRefs(g,e,"targets",15)) {
        const auto& target=g.elements.at(id);
        if(g.strings.at(target.type)!="DmeConstraintTarget")throw std::runtime_error("invalid point target");
        auto dag=hierarchyRefs(g,target,"target",1);
        const auto* weight=namedAttribute(g,target,"targetWeight");
        const auto* offset=namedAttribute(g,target,"vecOffset");
        if(!weight||weight->type!=3||weight->values.size()!=1||weight->values[0].wire.size()!=4)
            throw std::runtime_error("invalid target weight");
        if(!offset||offset->type!=10||offset->values.size()!=1||offset->values[0].wire.size()!=12)
            throw std::runtime_error("invalid target offset");
        RecoveredPointConstraintTarget decoded{};
        std::memcpy(&decoded.weight,weight->values[0].wire.data(),4);
        std::memcpy(decoded.offset.data(),offset->values[0].wire.data(),12);
        decoded.hasResolvedDag=!dag.empty();
        if(decoded.hasResolvedDag){auto world=hierarchy.world(dag[0]);for(unsigned c=0;c<3;++c)decoded.worldPosition[c]=world[c][3];}
        targets.push_back(decoded);
    }
    return accumulateRecoveredPointConstraint(targets,initializedGlobalVector);
}
