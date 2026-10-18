#pragma once
#include "graph_orientation_constraint.h"
#include "quaternion_inverse.h"
#include "quaternion_interpolation.h"
#include "session_quaternion_runtime.h"
inline bool executeSessionOrientationConstraint(Graph&g,GraphTransformHierarchy&h,
    uint32_t constraint,const std::array<float,4>&initialOutput,
    SessionAttributeChanges*changes=nullptr) {
    if(&h.graph!=&g)throw std::runtime_error("orientation hierarchy mismatch");
    const auto& e=g.elements.at(constraint);
    auto slaves=hierarchyRefs(g,e,"slave",1);
    if(slaves.empty())throw std::runtime_error("missing orientation slave");
    const auto& slave=g.elements.at(slaves[0]);
    if(g.strings.at(slave.type)!="DmeConstraintSlave")throw std::runtime_error("invalid orientation slave");
    auto targets=hierarchyRefs(g,slave,"target",1);
    if(targets.empty())return false;
    h.cache.clear();
    const auto average=graphOrientationConstraintAverage(g,h,constraint,initialOutput);
    auto desired=average.orientation;
    if(average.influence<1.0f){
        const auto* p=namedAttribute(g,slave,"position");
        const auto* q=namedAttribute(g,slave,"orientation");
        if(!p||p->type!=10||p->values.size()!=1||p->values[0].wire.size()!=12||
           !q||q->type!=13||q->values.size()!=1||q->values[0].wire.size()!=16)
            throw std::runtime_error("invalid orientation slave transform");
        std::array<float,3> position{};std::array<float,4> orientation{};
        std::memcpy(position.data(),p->values[0].wire.data(),12);
        std::memcpy(orientation.data(),q->values[0].wire.data(),16);
        const auto pp=h.parent(targets[0],true),rp=h.parent(targets[0],false);
        const auto world=convertRecoveredLocalToWorld(recoveredQuaternionTransformMatrix(orientation,position),
            h.world(pp),h.world(rp),pp==rp);
        desired=interpolateRecoveredQuaternion(extractRecoveredTransformOrientationCandidate(world),
            desired,average.influence,false);
    }
    const auto parent=h.worldOrientation(h.parent(targets[0],false));
    const auto local=multiplyRecoveredAlignedQuaternions(invertRecoveredQuaternion(parent),desired);
    auto transforms=hierarchyRefs(g,g.elements.at(targets[0]),"transform",1);
    if(transforms.empty())throw std::runtime_error("missing orientation destination transform");
    runtimeWriteQuaternion(g,{transforms[0],"orientation",0,13},local,changes);
    h.cache.clear();
    return true;
}
