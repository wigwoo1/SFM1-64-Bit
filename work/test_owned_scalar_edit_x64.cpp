#include "reference_owned_scene.h"
#include <iostream>
int main(){try{static_assert(sizeof(void*)==8,"x64 required");size_t cases=0;
 for(unsigned bits=0;bits<32;++bits)for(int phase=0;phase<8;++phase)for(unsigned target=0;target<4;++target)for(unsigned reject=0;reject<2;++reject){
 RecoveredOwnedSceneCandidate s;auto&o=s.create({1,0,0,0});auto&old=s.create({2,0,0,0});auto&next=s.create({3,0,0,0});auto&a=s.attach(o,1,{old.record.handle});a.flags=uint16_t(1|((bits&1)?0x20:0)|((bits&2)?0x80:0)|((bits&4)?0x2000:0)|((bits&8)?0x400:0));o.registration.state=(bits&16)?0:1;
 uint32_t incoming=target==0?old.record.handle:target==1?next.record.handle:target==2?0xffffffffu:0x7fc00000u;
 o.record.flags|=1;old.record.flags|=1;next.record.flags|=1;auto beforeFlags=a.flags;auto beforeState=o.registration.state;size_t classCalls=0;
 bool expected=target!=0&&!(target==1&&reject)&&attributeMutationPermitted(beforeFlags,phase);
 bool changed=s.setScalarReference(a.token,incoming,phase,42,[&](uint32_t h,uint32_t cls){if(h!=next.record.handle||cls!=42)throw std::runtime_error("class callback");++classCalls;return !reject;});
 if(changed!=expected||a.owned->front()!=(expected?incoming:old.record.handle)||classCalls!=(target==1?1u:0u))throw std::runtime_error("guard/storage");
 if(a.flags!=(expected?uint16_t(beforeFlags|0x4080):beforeFlags)||o.registration.state!=(expected?uint8_t(beforeState|0x13):beforeState))throw std::runtime_error("mutation state");
 if(s.queues.dirty.handles.size()!=(expected?1u:0u)||o.flags!=(expected?3u:0u))throw std::runtime_error("dirty state");
 auto oldTokens=s.references.nodes.tokens(old.record);auto newTokens=s.references.nodes.tokens(next.record);if(oldTokens.size()!=(expected?0u:1u)||newTokens.size()!=((expected&&target==1)?1u:0u))throw std::runtime_error("reverse links");
 if(!oldTokens.empty()&&oldTokens[0]!=a.token)throw std::runtime_error("old token");if(!newTokens.empty()&&newTokens[0]!=a.token)throw std::runtime_error("new token");
 s.cleanup({},[](auto&,bool){},[](auto&){});if(s.size()||s.attributes.size()||s.references.nodes.allocationCount())throw std::runtime_error("cleanup");++cases;
 }
 std::cout<<"scalar_edit_cases="<<cases<<" guard_state_reverse_teardown=passed\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
