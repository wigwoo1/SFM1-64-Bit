#include "registered_operator_build.h"
#include "expression_operator_dirty_candidate.h"
#include "element_mutation_state.h"
#include "attribute_mutation_gate.h"
#include "reference_owned_scene.h"
#include <iostream>
int main(){try{OperatorAttributeRegistration r;r.add(10,{100},{200});r.add(20,{200},{});r.add(30,{300},{});std::map<uintptr_t,uint16_t> flags{{100,uint16_t(0x4000)},{200,uint16_t(0x4000)},{300,uint16_t(0x4000)}};std::vector<uintptr_t> prepared;std::vector<uintptr_t> assigned;
 auto result=buildRegisteredOperators(r,{0,1,2},[](uintptr_t id){return id==10;},[&](uintptr_t id){prepared.push_back(id);},[&](uintptr_t id)->uint16_t&{return flags.at(id);},[&](uintptr_t id,size_t index){if(index!=assigned.size())throw std::runtime_error("index mismatch");assigned.push_back(id);});
 if(result.cycle||prepared!=std::vector<uintptr_t>{10}||assigned!=std::vector<uintptr_t>{30,10,20}||flags[100]||flags[200]||flags[300])throw std::runtime_error("build integration mismatch");
 assigned.clear();result=buildRegisteredOperators(r,{0,1,2},[](uintptr_t){return false;},[](uintptr_t){},[&](uintptr_t id)->uint16_t&{return flags.at(id);},[&](uintptr_t id,size_t){assigned.push_back(id);});if(!assigned.empty()||!result.identities.empty())throw std::runtime_error("dirty consumption mismatch");
 for(unsigned raw=0;raw<256;++raw){uint8_t state=uint8_t(raw);if(expressionOperatorDirtyCandidate(state)!=bool(raw&2))throw std::runtime_error("expression dirty predicate mismatch");clearExpressionOperatorDirtyCandidate(state);if(state!=(raw&0xfd)||expressionOperatorDirtyCandidate(state))throw std::runtime_error("expression dirty clear mismatch");}
 std::map<uintptr_t,uint8_t>operatorFlags{{10,uint8_t(0xfe)},{20,uint8_t(0xa0)},{30,uint8_t(1)}};assigned.clear();
 result=buildRegisteredOperators(r,{0,1,2},[&](uintptr_t id){return expressionOperatorDirtyCandidate(operatorFlags.at(id));},[&](uintptr_t id){clearExpressionOperatorDirtyCandidate(operatorFlags.at(id));},[&](uintptr_t id)->uint16_t&{return flags.at(id);},[&](uintptr_t id,size_t){assigned.push_back(id);});
 if(result.cycle||assigned!=std::vector<uintptr_t>{10,20}||operatorFlags.at(10)!=0xfc||operatorFlags.at(20)!=0xa0||operatorFlags.at(30)!=1)throw std::runtime_error("expression state selection integration mismatch");
 for(unsigned raw=0;raw<256;++raw){uint8_t state=uint8_t(raw);clearExpressionOperatorDirtyCandidate(state);unsigned queued=0;auto enqueue=[&](uint32_t handle){if(handle!=0x12345678u)throw std::runtime_error("wrong owner handle");++queued;};markElementMutation(state,0x12345678u,enqueue);markElementMutation(state,0x12345678u,enqueue);
  if(state!=((raw&0xfd)|3)||!expressionOperatorDirtyCandidate(state)||queued!=unsigned(!(raw&1)))throw std::runtime_error("expression dirty revival/queue mismatch");std::vector<uint16_t>attributeFlags{0x4080,0xffff};clearElementMutation(state,attributeFlags);if(state!=(((raw&0xfd)|3)&0xfe)||!expressionOperatorDirtyCandidate(state)||attributeFlags!=std::vector<uint16_t>{0x4000,0xff7f})throw std::runtime_error("owner/operator clear distinction mismatch");}
 unsigned editChecks=0;
 for(int phase=0;phase<8;++phase)for(unsigned bits=0;bits<16;++bits)for(unsigned ownerDirty=0;ownerDirty<2;++ownerDirty){
  uint16_t attr=uint16_t(((bits&1)?0x20:0)|((bits&2)?0x400:0)|((bits&4)?0x2000:0)|((bits&8)?0x80:0));const auto initial=attr;uint8_t owner=uint8_t(0xa0|ownerDirty);unsigned queues=0,prepares=0;std::vector<uintptr_t>order;
  const bool permitted=!(initial&0x20)&&(phase==0||phase==1||phase==2||(phase==4&&!(initial&0x400)));
  const bool accepted=beginAttributeMutation(attr,phase,[&](){++prepares;},[&](){markElementMutation(owner,10,[&](uint32_t){++queues;});});
  if(accepted!=permitted||attr!=(permitted?uint16_t(initial|0x4080):initial)||queues!=unsigned(permitted&&!ownerDirty)||prepares!=unsigned(permitted&&!(initial&0x80)&&(initial&0x2000)))throw std::runtime_error("edit gate lifecycle mismatch");
  OperatorAttributeRegistration editRegistration;editRegistration.add(10,{100},{200});editRegistration.add(20,{200},{});std::map<uintptr_t,uint16_t>editFlags{{100,attr},{200,0}};
  auto schedule=buildRegisteredOperators(editRegistration,{0,1},[&](uintptr_t id){return id==10&&expressionOperatorDirtyCandidate(owner);},[&](uintptr_t){clearExpressionOperatorDirtyCandidate(owner);},[&](uintptr_t id)->uint16_t&{return editFlags.at(id);},[&](uintptr_t id,size_t){order.push_back(id);});
  if(schedule.cycle||order!=(permitted?std::vector<uintptr_t>{10,20}:std::vector<uintptr_t>{})||expressionOperatorDirtyCandidate(owner)||(editFlags.at(100)&0x4000))throw std::runtime_error("edit to schedule mismatch");
  if(permitted){order.clear();beginAttributeMutation(attr,phase,[](){},[&](){markElementMutation(owner,10,[&](uint32_t){++queues;});});if(!expressionOperatorDirtyCandidate(owner)||queues!=unsigned(!ownerDirty))throw std::runtime_error("repeat edit failed to revive operator without duplicate queue");}
  ++editChecks;
 }
 unsigned ownedChecks=0;
 for(int phase=0;phase<8;++phase)for(unsigned bits=0;bits<16;++bits){
  RecoveredOwnedSceneCandidate scene;RecoveredPendingIdentityCandidate identity{};identity[0]=1;auto&owner=scene.create(identity);auto&attribute=scene.attach(owner,1,{0xffffffffu});owner.registration.state=0xa0;
  attribute.flags=uint16_t(1|((bits&1)?0x20:0)|((bits&2)?0x400:0)|((bits&4)?0x2000:0)|((bits&8)?0x80:0));const bool permitted=attributeMutationPermitted(attribute.flags,phase);
  scene.invalidate(attribute.token,phase);if(expressionOperatorDirtyCandidate(owner.registration.state)!=permitted||scene.queues.registration.handles.size()!=size_t(permitted))throw std::runtime_error("owned invalidation dirty state mismatch");
  unsigned dispatched=0;scene.dispatchResolveQueue([&](auto*resolved){if(resolved!=&owner)throw std::runtime_error("wrong owned callback");++dispatched;});if(dispatched!=unsigned(permitted)||(owner.registration.state&1)||expressionOperatorDirtyCandidate(owner.registration.state)!=permitted)throw std::runtime_error("owned dispatch consumed operator dirty bit");
  OperatorAttributeRegistration registration;registration.add(owner.record.handle,{attribute.token},{});std::vector<uintptr_t>order;
  auto schedule=buildRegisteredOperators(registration,{0},[&](uintptr_t){return expressionOperatorDirtyCandidate(owner.registration.state);},[&](uintptr_t){clearExpressionOperatorDirtyCandidate(owner.registration.state);},[&](uintptr_t)->uint16_t&{return attribute.flags;},[&](uintptr_t id,size_t){order.push_back(id);});
  if(schedule.cycle||order.size()!=size_t(permitted)||expressionOperatorDirtyCandidate(owner.registration.state)||(attribute.flags&0x4000))throw std::runtime_error("owned attribute operator selection mismatch");
  ++ownedChecks;
 }
 std::cout<<"registered_operator_build_checks=3 expression_flag_checks=256 mutation_lifecycle_checks=256 edit_schedule_checks="<<editChecks<<" owned_edit_schedule_checks="<<ownedChecks<<"\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
