#pragma once
#include "graph_ik_solution.h"
#include "session_quaternion_runtime.h"
#include "session_vector_runtime.h"
#include "quaternion_interpolation.h"
#include "graph_transform_control_default.h"
template<class WriteQuaternion,class WriteVector>bool executeSessionIkWithWritersCandidate(Graph&g,GraphTransformHierarchy&h,uint32_t id,
 const std::array<float,3>&globalZero,const std::array<const std::array<float,3>*,3>&defaults,
 WriteQuaternion writeQuaternion,WriteVector writeVector){
 const auto inputs=readRecoveredGraphIkInputs(g,id);
 for(auto dag:inputs.dags)if(dag==0xffffffffu)return false;
 const auto solved=graphIkSolutionCandidate(g,h,id,globalZero,defaults);
 std::array<uint32_t,3>transforms{};std::array<std::array<float,4>,2>blended{};
 for(unsigned i=0;i<3;++i){const auto refs=hierarchyRefs(g,g.elements.at(inputs.dags[i]),"transform",1);
  if(refs.empty())throw std::runtime_error("missing IK write transform");transforms[i]=refs[0];}
 for(unsigned i=0;i<2;++i){const auto*a=namedAttribute(g,g.elements.at(inputs.slaves[i]),"orientation");
  if(!a||a->type!=13||a->values.size()!=1||a->values[0].wire.size()!=16)throw std::runtime_error("invalid IK saved slave orientation");
  std::array<float,4>q{};std::memcpy(q.data(),a->values[0].wire.data(),16);
  blended[i]=interpolateRecoveredQuaternion(q,i?solved.rotations.mid:solved.rotations.start,solved.influence,false);}
 for(unsigned i=0;i<2;++i)writeQuaternion(transforms[i],blended[i]);
 for(unsigned i=0;i<3;++i)if(defaults[i]&&*defaults[i]!=globalZero)
  writeVector(transforms[i],*defaults[i]);
 h.cache.clear();return true;
}
inline bool executeSessionIkCandidate(Graph&g,GraphTransformHierarchy&h,uint32_t id,
 const std::array<float,3>&globalZero,const std::array<const std::array<float,3>*,3>&defaults,SessionAttributeChanges*changes=nullptr){
 return executeSessionIkWithWritersCandidate(g,h,id,globalZero,defaults,[&](uint32_t t,const auto&q){runtimeWriteQuaternion(g,{t,"orientation",0,13},q,changes);},[&](uint32_t t,const auto&v){runtimeWriteVector(g,{t,"position",0,10},v,changes);});
}
// Controls must already be selected using the native ordering policy.
inline bool executeSessionIkWithSelectedControlsCandidate(Graph&g,GraphTransformHierarchy&h,uint32_t id,
 const std::array<float,3>&globalZero,const std::array<uint32_t,3>&controls,
 SessionAttributeChanges*changes=nullptr){
 std::array<std::array<float,3>,3>values{};
 std::array<const std::array<float,3>*,3>defaults{};
 for(unsigned i=0;i<3;++i)if(controls[i]!=0xffffffffu){
  values[i]=graphSelectedControlDefaultPositionCandidate(g,controls[i],globalZero);
  defaults[i]=&values[i];
 }
 return executeSessionIkCandidate(g,h,id,globalZero,defaults,changes);
}
