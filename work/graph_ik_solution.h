#pragma once
#include "graph_ik_inputs.h"
#include "graph_ik_parent_frame.h"
#include "graph_point_constraint.h"
#include "ik_prepared_solution.h"
#include "ik_saved_pole.h"
struct RecoveredGraphIkSolutionCandidate{RecoveredIkPreparedSolutionCandidate rotations;float influence;};
// Explicit selected defaults: graph discovery/order remains caller responsibility.
inline RecoveredGraphIkSolutionCandidate graphIkSolutionCandidate(const Graph&g,
 GraphTransformHierarchy&h,uint32_t id,const std::array<float,3>&globalZero,
 const std::array<const std::array<float,3>*,3>&defaults){
 const auto inputs=readRecoveredGraphIkInputs(g,id);
 std::array<std::array<float,3>,3>positions{};
 for(unsigned i=0;i<3;++i){if(inputs.dags[i]==0xffffffffu)throw std::runtime_error("missing IK joint target");
  const auto refs=hierarchyRefs(g,g.elements.at(inputs.dags[i]),"transform",1);
  if(refs.empty())throw std::runtime_error("missing IK joint transform");
  graphLocalTransformMatrix(g,refs[0]);
  std::memcpy(positions[i].data(),namedAttribute(g,g.elements.at(refs[0]),"position")->values[0].wire.data(),12);}
 auto root=positions[0];if(defaults[0]&&*defaults[0]!=globalZero)root=*defaults[0];
 const auto length=[](const std::array<float,3>&v){return std::sqrt((v[0]*v[0]+v[1]*v[1])+v[2]*v[2]);};
 float upper=length(positions[1]),lower=length(positions[2]);
 if(defaults[1]){float candidate=length(*defaults[1]);if(candidate>0)upper=candidate;}
 if(defaults[2]){float candidate=length(*defaults[2]);if(candidate>0)lower=candidate;}
 h.cache.clear();const auto inverse=recoveredIkCoordinateInverseCandidate(graphIkParentFrameCandidate(g,h,inputs.dags[0]));
 const auto average=graphPointConstraintAverage(g,h,id,globalZero);
 const auto target=transformRecoveredIkPointCandidate(average.position,inverse);
 std::array<float,3>pole{};
 if(inputs.poleVectorTarget!=0xffffffffu){const auto world=h.world(inputs.poleVectorTarget);
  pole=transformRecoveredIkPointCandidate({world[0][3],world[1][3],world[2][3]},inverse);
  for(unsigned i=0;i<3;++i)pole[i]=pole[i]-root[i];normalizeRecoveredIkVectorCandidate(pole);
 }else pole=recoveredIkSavedPoleCandidate(inputs.poleVector,inverse);
 return {recoveredIkPreparedSolutionCandidate(root,target,pole,upper,lower,inputs.startOffset,inputs.midOffset),average.influence};
}
