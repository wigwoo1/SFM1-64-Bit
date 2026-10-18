#pragma once
#include "graph_point_constraint.h"
#include "session_transform_runtime.h"
#include "point_constraint_blend.h"
inline bool executeSessionPointConstraint(Graph&g,GraphTransformHierarchy&hierarchy,
    uint32_t constraint,const std::array<float,3>&initializedGlobalVector,
    SessionAttributeChanges*changes=nullptr) {
    const auto& e=g.elements.at(constraint);
    auto slaves=hierarchyRefs(g,e,"slave",1);
    if(slaves.empty())throw std::runtime_error("missing point constraint slave");
    const auto& slave=g.elements.at(slaves[0]);
    if(g.strings.at(slave.type)!="DmeConstraintSlave")throw std::runtime_error("invalid point slave");
    auto targets=hierarchyRefs(g,slave,"target",1);
    if(targets.empty())return false;
    hierarchy.cache.clear();
    const auto average=graphPointConstraintAverage(g,hierarchy,constraint,initializedGlobalVector);
    auto result=average.position;
    if(average.influence<1.0f) {
        const auto* p=namedAttribute(g,slave,"position");const auto* q=namedAttribute(g,slave,"orientation");
        if(!p||p->type!=10||p->values.size()!=1||p->values[0].wire.size()!=12||
           !q||q->type!=13||q->values.size()!=1||q->values[0].wire.size()!=16)
            throw std::runtime_error("invalid slave transform");
        std::array<float,3> position{};std::array<float,4> orientation{};
        std::memcpy(position.data(),p->values[0].wire.data(),12);std::memcpy(orientation.data(),q->values[0].wire.data(),16);
        const auto local=recoveredQuaternionTransformMatrix(orientation,position);
        const auto pp=hierarchy.parent(targets[0],true),rp=hierarchy.parent(targets[0],false);
        const auto world=convertRecoveredLocalToWorld(local,hierarchy.world(pp),hierarchy.world(rp),pp==rp);
        result=blendRecoveredPointConstraintPosition(average.position,average.influence,
            {world[0][3],world[1][3],world[2][3]});
    }
    writeSessionWorldPosition(g,hierarchy,targets[0],result,changes);
    return true;
}
