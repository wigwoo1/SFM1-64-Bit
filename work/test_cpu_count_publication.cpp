#include <Windows.h>
#include "studio_cpu_count_publication_candidate.h"
#include "native_cpu_topology_candidate.h"
#include "native_cpu_mask_services_candidate.h"
#include "studio_d3d9_creation_flags_candidate.h"
#include "native_cpu_creation_flag_services_candidate.h"
#include <cstdio>
#include <thread>
#include <vector>
struct Services{unsigned logical,backend;bool amd;unsigned calls=0;unsigned logicalCount(){return logical;}bool isAMD(){return amd;}unsigned backendCount(){++calls;return backend;}};
struct Flags{int mode;unsigned value;int queueMode(){return mode;}unsigned cpuByte6(){return value;}};
int main(){unsigned cases=0;
 for(bool amd:{false,true})for(unsigned logical:{0u,1u,2u,24u,255u,256u,257u})for(unsigned backend:{0u,1u,2u,255u,256u,257u}){
  Services services{logical,backend,amd};auto result=studioCpuCountPublicationCandidate(services);unsigned l=logical&255u,p=amd?l:backend&255u;if(!l&&!p)l=p=1;
  if(result.logical!=l||result.physical!=p||result.lowerPhysical!=(p<l)||services.calls!=(amd?0u:1u))return 1;
  for(int mode:{-2,-1,0,2}){Flags flags{mode,result.physical};unsigned expected=mode==2||((mode==-2||mode==-1)&&p>=2)?0x44:0x40;if(studioD3D9CreationFlagsCandidate(0x40,flags)!=expected)return 2;}++cases;
 }
 SYSTEM_INFO info{};GetSystemInfo(&info);NativeCpuMaskServicesCandidate cpuid;auto vendor=cpuid.basic;bool amd=vendor[1]==0x68747541u&&vendor[2]==0x444d4163u&&vendor[3]==0x69746e65u;
 NativeCpuTopologyQueryCandidate query;auto topology=nativeCpuTopologyCandidate(query);if(!amd&&!topology.available)return 3;
 Services live{info.dwNumberOfProcessors,topology.cores,amd};auto result=studioCpuCountPublicationCandidate(live);Flags flags{-2,result.physical};
 std::printf("cpu_count_publication_controlled_cases=%u live_logical_byte=%u live_physical_byte=%u live_creation_flags=%u amd=%u affinity_changed=0\n",cases,result.logical,result.physical,studioD3D9CreationFlagsCandidate(0x40,flags),amd?1u:0u);
 int mode=2;NativeCpuCreationFlagServicesCandidate binding{mode};if(studioD3D9CreationFlagsCandidate(0x40,binding)!=0x44||binding.initializations)return 4;
 mode=-2;auto first=studioD3D9CreationFlagsCandidate(0x40,binding);auto second=studioD3D9CreationFlagsCandidate(0x40,binding);if(first!=second||binding.initializations!=1||binding.cpuCalls!=2||binding.counts->physical!=result.physical)return 5;
 std::printf("live_creation_flag_binding_lazy_cache=passed flags=%u\n",first);
 NativeCpuCreationFlagServicesCandidate concurrent{mode};std::atomic<unsigned> mismatches{0},ready{0};std::atomic<bool> start{false};std::vector<std::thread> workers;
 for(unsigned worker=0;worker<8;++worker)workers.emplace_back([&]{++ready;while(!start.load())std::this_thread::yield();for(unsigned i=0;i<128;++i)if(studioD3D9CreationFlagsCandidate(0x40,concurrent)!=first)++mismatches;});
 while(ready.load()!=8)std::this_thread::yield();start=true;for(auto& worker:workers)worker.join();
 if(mismatches||concurrent.initializations!=1||concurrent.cpuCalls!=1024)return 6;
 std::printf("live_creation_flag_concurrent_cache_calls=1024 workers=8 initializations=1 mismatches=0\n");
 std::atomic<int> changingMode{-2};NativeAtomicCpuCreationFlagServicesCandidate changing{changingMode};mismatches=0;ready=0;start=false;workers.clear();
 // Initialize once, then exercise updates/readers independently of topology
 // discovery latency. Reads remain separate as in the recovered flag policy.
 changing.cpuByte6();
 for(unsigned worker=0;worker<8;++worker)workers.emplace_back([&]{++ready;while(!start.load())std::this_thread::yield();for(unsigned i=0;i<4096;++i){auto value=studioD3D9CreationFlagsCandidate(0x40,changing);if(value!=0x40&&value!=0x44)++mismatches;}});
 while(ready.load()!=8)std::this_thread::yield();start=true;
 for(unsigned i=0;i<4096;++i)changingMode.store(i%3==0?0:i%3==1?2:-2);
 for(auto& worker:workers)worker.join();if(mismatches||changing.initializations!=1)return 7;
 for(int value:{0,2,-2}){changingMode=value;unsigned expected=value==0?0x40:value==2?0x44:first;if(studioD3D9CreationFlagsCandidate(0x40,changing)!=expected)return 8;}
 std::printf("atomic_mode_concurrent_requests=32768 writers=1 readers=8 settled_values=passed\n");
}
