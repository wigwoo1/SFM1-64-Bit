#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "reference_owned_dmx_projection.h"
#include "expression_operator_dirty_candidate.h"
#include "owned_dmx_class_resolve_registry.h"
#include "registered_operator_build.h"
#include "owned_scalar_float_undo_journal.h"
#include <iostream>
int main(){try{static_assert(sizeof(void*)==8,"x64 required");unsigned cases=0;
 for(int phase=0;phase<8;++phase)for(unsigned flags=0;flags<16;++flags)for(uint32_t incoming: {0u,0x80000000u,0x3f800000u,0x7f800000u,0x7f800001u,0xffc12345u}){
 Graph g;g.strings={"DmeExpressionOperator","expression","input"};Element e{};e.id[0]=1;e.type=0;e.name=1;Value value;value.wire.resize(4);e.attributes.push_back({2,3,{value}});g.elements.push_back(e);RecoveredOwnedDmxReferenceProjectionCandidate<Graph> p(g,[](const std::string&,auto&){return 0xffffffffu;});auto*owner=p.scene.resolve(p.elementHandles[0]);owner->registration.state=0xa0;auto&attr=p.typedAttributeFlags[0][0];attr=uint16_t(3|((flags&1)?0x20:0)|((flags&2)?0x400:0)|((flags&4)?0x2000:0)|((flags&8)?0x80:0));const auto initial=attr;float f;std::memcpy(&f,&incoming,4);
 const bool changed=(incoming&0x7fffffffu)!=0;const bool permitted=!(initial&0x20)&&(phase==0||phase==1||phase==2||(phase==4&&!(initial&0x400)));const bool expected=changed&&permitted;
 RecoveredScalarFloatUndoPayloadCandidate undo;undo.owner=123;undo.name=456;if(p.setScalarFloat(0,"input",f,phase,&undo)!=expected)throw std::runtime_error("scalar mutation decision");uint32_t result;std::memcpy(&result,p.graph.elements[0].attributes[0].values[0].wire.data(),4);const auto stored=(incoming&0x7f800000u)==0x7f800000u&&(incoming&0x7fffffu)?incoming|0x400000u:incoming;
 if(result!=(expected?stored:0u)||attr!=(expected?uint16_t(initial|0x4080):initial)||expressionOperatorDirtyCandidate(owner->registration.state)!=expected||p.scene.queues.registration.handles.size()!=size_t(expected)||p.scene.queues.dirty.handles.size()!=size_t(expected))throw std::runtime_error("scalar payload and dirty lifecycle");
 if(!expected&&(undo.owner!=123||undo.name!=456))throw std::runtime_error("rejected capture changed record");
 if(expected){if(undo.owner!=p.elementHandles[0]||undo.name!=2)throw std::runtime_error("undo identity capture");OwnedDmxClassResolveRegistryCandidate<decltype(p)>registry;auto dispatch=registry.dispatch(p);if(dispatch.unregistered!=1||dispatch.registered||owner->registration.state!=0xb2||(attr&0x80))throw std::runtime_error("scalar resolve dispatch");OperatorAttributeRegistration registration;registration.add(p.elementHandles[0],{1},{});unsigned selected=0;auto order=buildRegisteredOperators(registration,{0},[&](uintptr_t){return expressionOperatorDirtyCandidate(owner->registration.state);},[&](uintptr_t){clearExpressionOperatorDirtyCandidate(owner->registration.state);},[&](uintptr_t)->uint16_t&{return attr;},[&](uintptr_t,size_t){++selected;});if(order.cycle||selected!=1||owner->registration.state!=0xb0||(attr&0x4000))throw std::runtime_error("scalar edit operator selection");
  if(!p.setScalarFloat(0,"input",2.f,phase)||!expressionOperatorDirtyCandidate(owner->registration.state)||p.scene.queues.registration.handles.size()!=1)throw std::runtime_error("second scalar edit after dispatch");
  if(!p.replayScalarFloatUndo(undo,true,0)||wireFloat(p.graph.elements[0].attributes[0].values[0])!=0.f)throw std::runtime_error("owned scalar undo");if(!p.replayScalarFloatUndo(undo,false,0))throw std::runtime_error("owned scalar redo");uint32_t restored;std::memcpy(&restored,p.graph.elements[0].attributes[0].values[0].wire.data(),4);if(restored!=stored)throw std::runtime_error("owned scalar redo bits");attr|=0x20;if(p.replayScalarFloatUndo(undo,true,0))throw std::runtime_error("protected owned scalar undo");
 }
 p.scene.cleanup({},[](auto&,bool){},[](auto&){});if(p.setScalarFloat(0,"input",1.f,0))throw std::runtime_error("stale scalar owner");++cases;
 }
 {Graph g;g.strings={"DmeExpressionOperator","expression","input"};Element e{};e.id[0]=1;e.type=0;e.name=1;Value value;value.wire.resize(4);e.attributes.push_back({2,3,{value}});g.elements.push_back(e);RecoveredOwnedDmxReferenceProjectionCandidate<Graph>p(g,[](const std::string&,auto&){return 0xffffffffu;});OwnedScalarFloatUndoJournalCandidate journal;journal.beginBoundary();
 for(float v:{1.f,2.f})if(!editOwnedScalarFloatWithUndoCandidate(p,journal,0,"input",v,0,123))throw std::runtime_error("journal edit");
 if(journal.undo.size()!=2||replayOwnedScalarFloatJournalCandidate(p,journal,true,0)!=2||wireFloat(p.graph.elements[0].attributes[0].values[0])!=0.f||journal.flags!=8)throw std::runtime_error("journal grouped undo");
 if(replayOwnedScalarFloatJournalCandidate(p,journal,false,0)!=2||wireFloat(p.graph.elements[0].attributes[0].values[0])!=2.f||journal.undo.size()!=2)throw std::runtime_error("journal grouped redo");
 replayOwnedScalarFloatJournalCandidate(p,journal,true,0);if(!editOwnedScalarFloatWithUndoCandidate(p,journal,0,"input",3.f,0,123)||!journal.redo.empty()||journal.undo.size()!=1)throw std::runtime_error("journal branch edit");
 if(!editOwnedScalarFloatWithUndoCandidate(p,journal,0,"input",4.f,0,0xffffffffu)||journal.undo.size()!=1)throw std::runtime_error("journal file gate");journal.flags=0;if(!editOwnedScalarFloatWithUndoCandidate(p,journal,0,"input",5.f,0,123)||journal.undo.size()!=1)throw std::runtime_error("journal disabled recording");
 }
 std::cout<<"owned_scalar_float_cases="<<cases<<"\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
