#include "studio_owner_post_reset_candidate.h"
#include <cassert>
#include <vector>
#include <cstdio>
struct Owner{uint32_t cleanupDepth;};
struct Services{
 bool own=true,main=true,available=true,mutate=false;Owner* owner;std::vector<unsigned> trace;uint32_t warned=0;
 bool inOwningThread(Owner&){trace.push_back(1);return own;}bool inMainThread(){trace.push_back(2);return main;}
 void deferPostReset(unsigned value){trace.push_back(value==7?3:4);}
 void forcedDepthWarning(uint32_t value,const char*){warned=value;trace.push_back(5);if(mutate)owner->cleanupDepth=99;}
 void unbalancedWarning(uint32_t value){warned=value;trace.push_back(6);}
 void unbalancedDiagnostic(){trace.push_back(7);if(mutate)owner->cleanupDepth=3;}
 void restoreOwner(Owner&,bool value){assert(value);trace.push_back(8);}
 bool notificationAvailable(){return available;}
 void beginPostNotification(uint32_t value){assert(value==0xfff5940f);trace.push_back(9);}
 void prepareRecreation(){trace.push_back(10);}void recreateFirst(Owner&){trace.push_back(11);}void recreateSecond(Owner&){trace.push_back(12);}void resumeCommandState(){trace.push_back(13);}void publishInterfaceFactory(){trace.push_back(14);if(mutate)available=false;}void endNotification(){trace.push_back(15);}
};
int main(){unsigned cases=0;
 for(bool own:{false,true})for(bool main:{false,true})for(uint32_t depth:{0u,1u,2u,0x7fffffffu,0x80000000u,0xffffffffu})for(bool restore:{false,true})for(bool force:{false,true})for(bool mutate:{false,true}){
  Owner owner{depth};Services s;s.owner=&owner;s.own=own;s.main=main;s.mutate=mutate;studioOwnerPostResetCandidate(owner,s,restore,force,nullptr);
  std::vector<unsigned> expected{1};if(own)expected.push_back(2);
  if(!own||!main){if(restore)expected.push_back(3);expected.push_back(4);assert(owner.cleanupDepth==depth);}
  else if(!force&&depth!=1){expected.push_back(6);expected.push_back(7);const uint32_t after=mutate?3u:depth-1u;assert(owner.cleanupDepth==((after&0x80000000u)?0u:after));assert(s.warned==depth-1u);}
  else{if(force&&depth>1&&depth<0x80000000u){expected.push_back(5);assert(s.warned==depth);}if(restore)expected.push_back(8);for(unsigned v:{9u,10u,11u,12u,13u,14u})expected.push_back(v);if(!mutate)expected.push_back(15);assert(owner.cleanupDepth==0);}
  assert(s.trace==expected);++cases;
 }
 std::printf("post_reset_candidate_cases=%u passed\n",cases);
}
