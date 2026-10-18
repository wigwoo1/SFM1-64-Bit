#include "studio_owner_reset_cleanup_candidate.h"
#include <cassert>
#include <cstdint>
#include <vector>
struct Owner{unsigned char flags=0xff;uint32_t cleanupDepth=0;};
struct Services{
 bool owning=true,main=true,available=true,removeNotification=false;std::vector<unsigned> trace;unsigned checks=0;uint32_t warning=0;
 bool inOwningThread(Owner&){trace.push_back(1);return owning;}
 bool inMainThread(){trace.push_back(2);return main;}
 void deferCleanup(bool value){assert(value);trace.push_back(3);}
 void nestedWarning(uint32_t value){warning=value;trace.push_back(4);}
 void nestedDiagnostic(){trace.push_back(5);}
 bool notificationAvailable(){++checks;return available;}
 void beginNotification(uint32_t value){assert(value==0xfff5940f);trace.push_back(6);if(removeNotification)available=false;}
 void endNotification(){trace.push_back(7);}
 void queryCleanup(Owner&){trace.push_back(8);}
 void cleanupFirst(Owner&){trace.push_back(9);}
 void cleanupSecond(Owner&){trace.push_back(10);}
 void setCleanupMode(unsigned value){trace.push_back(11+value);}
 void finishCommandState(){trace.push_back(14);}
 void publishCleanupEnabled(bool value){trace.push_back(15+static_cast<unsigned>(value));}
};
int main(){unsigned cases=0;
 for(bool owning:{false,true})for(bool main:{false,true})for(uint32_t depth:{0u,1u,0x7fffffffu,0xffffffffu})for(bool enabled:{false,true}){
  Owner o;o.cleanupDepth=depth;Services s;s.owning=owning;s.main=main;
  studioOwnerResetCleanupCandidate(o,s,enabled);
  if(!owning||!main){assert(o.flags==0xdf&&o.cleanupDepth==depth&&s.checks==0);assert(s.trace==(owning?std::vector<unsigned>{1,2,3}:std::vector<unsigned>{1,3}));}
  else if(depth){assert(o.cleanupDepth==depth+1u&&s.warning==depth+1u&&o.flags==0xff&&s.checks==0);assert(s.trace==std::vector<unsigned>({1,2,4,5}));}
  else{assert(o.cleanupDepth==1&&o.flags==0xff&&s.checks==2);assert(s.trace==std::vector<unsigned>({1,2,6,7,8,9,10,enabled?13u:11u,14,enabled?16u:15u}));}
  ++cases;
 }
 for(bool present:{false,true}){Owner o;o.flags=0;Services s;s.available=present;s.removeNotification=true;studioOwnerResetCleanupCandidate(o,s,false);assert(o.flags==0x20&&s.checks==(present?2u:1u));assert(s.trace== (present?std::vector<unsigned>{1,2,6,8,9,10,11,14,15}:std::vector<unsigned>{1,2,8,9,10,11,14,15}));++cases;}
 return cases==34?0:1;
}

