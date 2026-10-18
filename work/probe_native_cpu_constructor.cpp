#define NOMINMAX
#include "native_cpu_constructor_services_candidate.h"
#include "studio_cpu_constructor_candidate.h"
#include "studio_cpu_id_count_candidate.h"
#include <cstdio>
int main(){static_assert(sizeof(void*)==8,"x64 required");
 // This executable samples only its own process. Refuse multi-group inputs
 // because primary-group masks cannot verify restoration of all-group state.
 USHORT count=64;USHORT groups[64]{};if(!GetProcessGroupAffinity(GetCurrentProcess(),&count,groups))return 1;
 if(count!=1){std::printf("live_cpu_constructor_available=0 process_groups=%u reason=multi_group_unverified\n",count);return 0;}
 DWORD_PTR before=0,system=0,after=0,afterSystem=0;GROUP_AFFINITY threadBefore{},threadAfter{};
 if(!GetProcessAffinityMask(GetCurrentProcess(),&before,&system)||!GetThreadGroupAffinity(GetCurrentThread(),&threadBefore))return 2;
 NativeCpuConstructorServicesCandidate services;auto result=studioCpuConstructorCandidate(services);
 if(!GetProcessAffinityMask(GetCurrentProcess(),&after,&afterSystem)||!GetThreadGroupAffinity(GetCurrentThread(),&threadAfter))return 3;
 bool restored=before==after&&system==afterSystem&&threadBefore.Mask==threadAfter.Mask&&threadBefore.Group==threadAfter.Group;
 unsigned cores=result.valid?studioCpuIdCountCandidate(result.ids.data(),static_cast<uint8_t>(result.ids.size()),result.masks[5],result.masks[7],result.masks[2]):0;
 std::printf("live_cpu_constructor_available=%u sampled_ids=%zu distinct_legacy_ids=%u affinity_restored=%u restoration_failed=%u process_groups=%u\n",result.valid?1u:0u,result.ids.size(),cores,restored?1u:0u,result.restorationFailed?1u:0u,count);
 return restored&&!result.restorationFailed?0:4;
}
