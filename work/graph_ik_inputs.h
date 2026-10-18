#pragma once
#include "clip_hierarchy.h"
#include <array>
struct RecoveredGraphIkInputs{
 std::array<uint32_t,3> slaves{},dags{};
 std::array<float,4> startOffset{},midOffset{};
 std::array<float,3> poleVector{};
 uint32_t poleVectorTarget=0xffffffffu;
};
inline RecoveredGraphIkInputs readRecoveredGraphIkInputs(const Graph&g,uint32_t id){
 const auto&e=g.elements.at(id);
 if(g.strings.at(e.type)!="DmeRigIKConstraintOperator")throw std::runtime_error("expected IK constraint");
 RecoveredGraphIkInputs out;const char* names[]={"startJoint","midJoint","endJoint"};
 for(unsigned i=0;i<3;++i){auto refs=hierarchyRefs(g,e,names[i],1);
  if(refs.empty())throw std::runtime_error("missing IK joint slave");out.slaves[i]=refs[0];
  const auto&s=g.elements.at(refs[0]);if(g.strings.at(s.type)!="DmeConstraintSlave")throw std::runtime_error("invalid IK slave type");
  auto dags=hierarchyRefs(g,s,"target",1);out.dags[i]=dags.empty()?0xffffffffu:dags[0];}
 const auto read=[&](const char* name,uint8_t type,void*dest,size_t bytes){
  const auto*a=namedAttribute(g,e,name);
  if(!a||a->type!=type||a->values.size()!=1||a->values[0].wire.size()!=bytes)throw std::runtime_error(std::string("invalid IK input ")+name);
  std::memcpy(dest,a->values[0].wire.data(),bytes);};
 read("startOffset",13,out.startOffset.data(),16);read("midOffset",13,out.midOffset.data(),16);
 read("poleVector",10,out.poleVector.data(),12);
 const auto pv=hierarchyRefs(g,e,"pvTarget",1);
 if(!pv.empty())out.poleVectorTarget=pv[0];
 return out;
}
