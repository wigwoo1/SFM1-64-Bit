#include "studio_constant_storage_restore_candidate.h"
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <unordered_set>
#include <initializer_list>
struct Allocation{
 unsigned fail,attempts=0,frees=0;std::unordered_set<void*> live;
 uint32_t count(unsigned){return 2;}
 void* allocate(uint32_t n){const unsigned attempt=attempts++;if(attempt==fail)return nullptr;void* p=std::malloc(n?n:1);assert(p&&live.insert(p).second);return p;}
 void free(void* p){assert(live.erase(p)==1);++frees;std::free(p);}
};
// Initialization callbacks reject any missing group buffer; this catches unsafe
// continuation after a failed replacement without dereferencing null memory.
struct Initialization{
 StudioConstantStorageOwnerCandidate& owner;unsigned submissions=0,copies=0;
 struct Caps{uint32_t count(unsigned){return 2;}}caps;
 Caps* capability(){return &caps;}
 uint32_t* staging(unsigned id){assert(owner.staging[id]);return static_cast<uint32_t*>(owner.staging[id]);}
 uint32_t* cache(unsigned id){assert(owner.primary[id]);return static_cast<uint32_t*>(owner.primary[id]);}
 void copy(uint32_t*,uint32_t*,uint32_t){++copies;}
 void submitFloat(bool,uint32_t,uint32_t,uint32_t*){++submissions;}
 void submitSubobject(unsigned,uint32_t,uint32_t*,uint32_t,uint32_t){++submissions;}
};
struct Tail{unsigned defaults=0,unbinds=0;void restoreVertexDefaults(uint32_t p){assert(p==0x40000000);++defaults;}void unbindShaders(){++unbinds;}};
static void cleanup(StudioConstantStorageOwnerCandidate& owner,Allocation& allocation){for(unsigned id=0;id<6;++id){if(owner.primary[id])allocation.free(owner.primary[id]);if(owner.staging[id])allocation.free(owner.staging[id]);}assert(allocation.live.empty());}
int main(){unsigned cases=0;
 for(bool present:{false,true})for(unsigned fail=0;fail<12;++fail){
  StudioConstantStorageOwnerCandidate owner;Allocation allocation{99};
  if(present)for(unsigned id=0;id<6;++id){owner.primary[id]=allocation.allocate(32);owner.staging[id]=allocation.allocate(32);}
  allocation.attempts=0;allocation.fail=fail;const auto oldPrimary=owner.primary,oldStaging=owner.staging;
  Initialization initialization{owner};Tail tail;
  assert(!studioConstantStorageRestoreCheckedCandidate(owner,allocation,initialization,tail));
  assert(allocation.attempts==fail+1&&allocation.frees==(present?fail+1:0));
  assert(initialization.submissions==(fail>=6?3u:0u)&&tail.defaults==0&&tail.unbinds==0);
  for(unsigned index=0;index<12;++index){const unsigned id=index/2;void* p=index%2?owner.staging[id]:owner.primary[id];void* old=index%2?oldStaging[id]:oldPrimary[id];if(index==fail)assert(!p);else if(index>fail)assert(p==old);else assert(p&&allocation.live.count(p));}
  // Retry the same partial owner. Every replacement must free only live memory;
  // successful completion must initialize both groups and execute both tails.
  allocation.attempts=0;allocation.fail=99;Initialization retry{owner};Tail retryTail;
  assert(studioConstantStorageRestoreCheckedCandidate(owner,allocation,retry,retryTail));
  assert(allocation.attempts==12&&retry.submissions==6&&retryTail.defaults==1&&retryTail.unbinds==1);
  cleanup(owner,allocation);++cases;
 }
 std::printf("constant_restore_allocation_failure_and_retry_cases=%u passed\n",cases);
}
