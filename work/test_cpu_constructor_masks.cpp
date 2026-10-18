#include "studio_cpu_constructor_masks_candidate.h"
#include "native_cpu_mask_services_candidate.h"
#include <fstream>
#include <vector>
#include <cstdio>
struct Services{
 bool intel;uint32_t maximum=4,extended=0x80000008u,total,cores;bool enabled=true;
 uint32_t maximumBasic(){return maximum;}uint32_t maximumExtended(){return extended;}bool isIntel(){return intel;}
 std::array<uint32_t,4> cpuid(uint32_t leaf,uint32_t){if(leaf==1)return {0,total<<16,0,enabled?0x10000000u:0u};if(leaf==4)return {(cores-1u)<<26,0,0,0};if(leaf==0x80000008u)return {0,0,cores-1u,0};throw 1;}
};
int main(){static_assert(sizeof(void*)==8,"x64 required");std::ifstream oracle("outputs/native-cpu-id-masks.bin",std::ios::binary);std::vector<std::array<uint8_t,8>> records(65025);oracle.read(reinterpret_cast<char*>(records.data()),records.size()*8);if(!oracle||oracle.peek()!=std::char_traits<char>::eof())return 1;
 unsigned cases=0;
 for(bool intel:{false,true})for(uint32_t total:{1u,2u,4u,16u,32u,64u,128u,255u})for(uint32_t cores:{1u,2u,4u,16u,32u,64u}){
  Services services{intel,4,0x80000008u,total,cores};std::array<uint8_t,8> result{};
  if(!studioCpuConstructorMasksCandidate(services,result)||result!=records[(total-1)*255+cores-1])return 2;++cases;
 }
 for(bool enabled:{false,true})for(uint32_t maximum:{0u,1u,3u}){Services services{true,maximum,0,16,4,enabled};std::array<uint8_t,8> result{};
  const unsigned total=maximum>=1&&enabled?16u:1u;
  if(!studioCpuConstructorMasksCandidate(services,result)||result!=records[(total-1)*255])return 3;++cases;
 }
 Services invalid{false,4,0x80000008u,255,256};std::array<uint8_t,8> result{};if(studioCpuConstructorMasksCandidate(invalid,result))return 4;
 NativeCpuMaskServicesCandidate live;if(live.cpuid(8,0x80000000u)!=live.cpuid(0x80000008u,0))return 5;
 bool available=studioCpuConstructorMasksCandidate(live,result);
 std::printf("x64_constructor_mask_oracle_composition_cases=%u zero_wrap_rejection=passed live_available=%u live_total_byte=%u live_core_byte=%u intel=%u affinity_changed=0\n",cases,available?1u:0u,result[0],result[1],live.isIntel()?1u:0u);return available?0:5;
}
