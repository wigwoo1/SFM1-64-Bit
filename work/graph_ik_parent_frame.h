#pragma once
#include "graph_transform_hierarchy.h"
#include "ik_parent_frame.h"
// Native DAG+64 is the disableOverride bool attribute, default false.
inline RecoveredMatrix3x4 graphIkParentFrameCandidate(const Graph&g,
 GraphTransformHierarchy&h,uint32_t dag){
 if(&h.graph!=&g)throw std::runtime_error("IK frame hierarchy mismatch");
 const auto&e=g.elements.at(dag);const auto it=h.ordinaryParents.find(dag);
 const auto ordinary=h.world(it==h.ordinaryParents.end()?0xffffffffu:it->second);
 if(h.overridesDisabled(dag))return ordinary;
 const auto flag=[&](const char* name){const auto*a=namedAttribute(g,e,name);if(!a)return false;
  if(a->type!=4||a->values.size()!=1||a->values[0].wire.size()!=1)throw std::runtime_error("invalid IK frame override flag");
  return a->values[0].wire[0]!=0;};
 const bool pos=flag("overridePos"),rot=flag("overrideRot");
 const auto parents=hierarchyRefs(g,e,"overrideParent",1);
 if(parents.empty()||(!pos&&!rot))return ordinary;
 const auto overrideWorld=h.world(parents[0]);
 if(pos&&rot)return overrideWorld;
 if(rot)return recoveredIkRotationOnlyParentFrameCandidate(ordinary,h.worldOrientation(parents[0]));
 const auto transforms=hierarchyRefs(g,e,"transform",1);if(transforms.empty())return ordinary;
 const auto&t=g.elements.at(transforms[0]);graphLocalTransformMatrix(g,transforms[0]);
 std::array<float,3>p{};std::array<float,4>q{};
 std::memcpy(p.data(),namedAttribute(g,t,"position")->values[0].wire.data(),12);
 std::memcpy(q.data(),namedAttribute(g,t,"orientation")->values[0].wire.data(),16);
 return recoveredIkPositionOnlyParentFrameCandidate(ordinary,
  {overrideWorld[0][3],overrideWorld[1][3],overrideWorld[2][3]},p,q);
}
