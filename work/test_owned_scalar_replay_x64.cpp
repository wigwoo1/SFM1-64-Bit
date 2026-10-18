#include "reference_owned_scene.h"
#include <iostream>
int main(){try{static_assert(sizeof(void*)==8,"x64 required");size_t cycles=0;
 for(unsigned seed=0;seed<256;++seed){RecoveredOwnedSceneCandidate s;auto&o=s.create({seed+1,1,0,0});auto&before=s.create({seed+1,2,0,0});auto&after=s.create({seed+1,3,0,0});auto&a=s.attach(o,1,{before.record.handle});o.record.flags|=1;before.record.flags|=1;after.record.flags|=1;RecoveredScalarReferenceUndoPayloadCandidate r{o.record.handle,7,before.record.handle,after.record.handle};auto find=[&](auto&owner,uint32_t name){return &owner==&o&&name==7?a.token:0xffffffffu;};auto matches=[](uint32_t,uint32_t){return true;};
 if(!s.setScalarReference(a.token,r.after,0,0xffffffffu,matches))throw std::runtime_error("initial edit");
 for(unsigned i=0;i<8;++i){if(!s.replayScalarReference(r,true,find,0,0xffffffffu,matches)||a.references()[0]!=r.before||s.references.nodes.tokens(before.record).size()!=1||!s.references.nodes.tokens(after.record).empty())throw std::runtime_error("undo");if(!s.replayScalarReference(r,false,find,0,0xffffffffu,matches)||a.references()[0]!=r.after||s.references.nodes.tokens(after.record).size()!=1||!s.references.nodes.tokens(before.record).empty())throw std::runtime_error("redo");++cycles;}
 a.flags|=0x20;if(s.replayScalarReference(r,true,find,0,0xffffffffu,matches)||a.references()[0]!=r.after)throw std::runtime_error("protected replay");a.flags&=uint16_t(~0x20);r.name=8;if(s.replayScalarReference(r,true,find,0,0xffffffffu,matches))throw std::runtime_error("missing name");r.name=7;
 if(!s.replayScalarReference(r,true,find,3,0xffffffffu,matches)||a.references()[0]!=r.after)throw std::runtime_error("normal phase gate");
 s.cleanup({},[](auto&,bool){},[](auto&){});if(s.replayScalarReference(r,true,find,0,0xffffffffu,matches)||s.size()||s.attributes.size()||s.references.nodes.allocationCount())throw std::runtime_error("stale owner teardown");
 }std::cout<<"owned_undo_redo_cycles="<<cycles<<" protected_missing_phase_stale_reverse_teardown=passed\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
