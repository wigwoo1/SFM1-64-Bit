#include "studio_cpu_constructor_candidate.h"
#include <fstream>
#include <cstring>
#include <cstdio>
struct Services{
 bool intel;unsigned total,cores;uintptr_t processMask,systemMask,threadMask=4;unsigned processCalls=0,threadCalls=0,sleeps=0;
 unsigned failure=0;
 unsigned maximumBasic(){return 4;}unsigned maximumExtended(){return 0x80000008u;}bool isIntel(){return intel;}
 std::array<uint32_t,4> cpuid(unsigned leaf,unsigned prefix){leaf|=prefix;std::array<uint32_t,4> output{};if(leaf==1){unsigned bit=0;auto mask=threadMask;while(mask>1){++bit;mask>>=1;}output[1]=(total<<16)|((bit+10u)<<24);output[3]=0x10000000u;}else if(leaf==4)output[0]=(cores-1)<<26;else if(leaf==0x80000008u)output[2]=cores-1;else throw 1;return output;}
 uintptr_t currentProcess(){return 0x100000111ull;}uintptr_t currentThread(){return 0x100000222ull;}
 bool affinityMasks(uintptr_t handle,uintptr_t& process,uintptr_t& system){if(handle!=currentProcess())throw 2;if(failure==1)return false;process=processMask;system=systemMask;return true;}
 bool setProcessAffinity(uintptr_t handle,uintptr_t mask){if(handle!=currentProcess())throw 3;++processCalls;if((failure==2&&processCalls==1)||(failure==5&&processCalls==2))return false;processMask=mask;return true;}
 uintptr_t setThreadAffinity(uintptr_t handle,uintptr_t mask){if(handle!=currentThread())throw 4;++threadCalls;if(failure==3&&threadCalls==2)return 0;auto previous=threadMask;threadMask=mask;return previous;}
 void yield(){++sleeps;if(failure==4&&sleeps==1)throw 99;}
};
int main(){static_assert(sizeof(void*)==8,"x64 required");std::ifstream oracle("outputs/native-cpu-constructor-grid.bin",std::ios::binary);if(!oracle)return 1;unsigned cases=0;
 for(bool vendor:{false,true})for(unsigned logical:{1u,16u,32u})for(unsigned cores:{1u,4u,8u})for(uintptr_t system:{1u,3u,5u,0x80000001u})for(bool restricted:{false,true}){
  Services services{vendor,logical,cores,restricted?1:system,system};auto result=studioCpuConstructorCandidate(services);if(!result.valid||result.ids.size()>32)return 2;
  std::array<uint32_t,16> record{},native{};std::memcpy(record.data(),result.masks.data(),8);std::memcpy(record.data()+2,result.ids.data(),result.ids.size());record[10]=static_cast<uint32_t>(result.ids.size());record[11]=static_cast<uint32_t>(services.processMask);record[12]=static_cast<uint32_t>(services.threadMask);record[13]=services.processCalls;record[14]=services.threadCalls;record[15]=services.sleeps;
  oracle.read(reinterpret_cast<char*>(native.data()),sizeof(native));if(!oracle||record!=native){std::printf("constructor_mismatch=%u\n",cases);return 3;}++cases;
 }
 if(oracle.peek()!=std::char_traits<char>::eof())return 4;
 Services wide{true,16,4,1,(uintptr_t{1}<<63)|1};auto result=studioCpuConstructorCandidate(wide);if(!result.valid||result.ids!=std::vector<uint8_t>{10,73}||wide.processMask!=1||wide.threadMask!=4||wide.processCalls!=2||wide.threadCalls!=3)return 5;
 for(unsigned failure=1;failure<=5;++failure){Services services{true,16,4,1,3};services.failure=failure;bool thrown=false;StudioCpuConstructorResultCandidate failed;
  try{failed=studioCpuConstructorCandidate(services);}catch(int value){if(value!=99)return 6;thrown=true;}
  if(failure==4){if(!thrown||services.processMask!=1||services.threadMask!=4)return 7;}
  else if(thrown||failed.valid||failed.restorationFailed!=(failure==5))return 8;
  if(failure!=5&&(services.processMask!=1||services.threadMask!=4))return 9;
 }
 std::printf("x64_native_cpu_constructor_matches=%u fullwidth_handle_and_bit63_case=passed checked_failure_cases=5 live_affinity_changed=0\n",cases);
}
