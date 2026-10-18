#include <array>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include "studio_index_buffer_creation_candidate.h"
#include "studio_profile_counter_candidate.h"
static void check(bool value){if(!value)std::abort();}
struct Ops{
 uintptr_t resourceBase=0x1234;unsigned count,dynamic,managed,result,secondResult,outputMask,creates=0,flushes=0,cleanups=0,logs=0,counter=7;
 bool rebindDevice=false;std::array<unsigned,2> createDevices{};unsigned cleanupDevice=0,deviceIndex=0;bool mutateMetadata=false;StudioIndexBufferCreationCandidate* state=nullptr;
 StudioProfileCounterCandidate profile;char counterName[32]="controlled buffer oracle";unsigned fallback=99;unsigned* allocationCounter=nullptr;unsigned profileThreadCalls=0,profileCompares=0;bool wrongThread=false,fullProfile=false,mutateRegistration=false;std::array<char,64> nameStorage{};unsigned allocations=0;
 void prepareCounter(){profile.count=1;profile.threadId=42;profile.values[0]=7;profile.groups[0]=2;profile.names[0]=counterName;allocationCounter=&profile.values[0];check(reinterpret_cast<uintptr_t>(allocationCounter)>UINT32_MAX);}
 unsigned currentThread(){++profileThreadCalls;return wrongThread?43u:42u;}
 int compare(const char* a,const char* b){++profileCompares;return std::strcmp(a,b);}
 char* allocate(std::size_t bytes){check(bytes==25&&++allocations==1);check(reinterpret_cast<uintptr_t>(nameStorage.data())>UINT32_MAX);return nameStorage.data();}
 void flush(){++flushes;if(rebindDevice)deviceIndex=flushes;}
 uintptr_t device(){return uintptr_t{0x100000012ull}+deviceIndex;}
 unsigned create(uintptr_t d,unsigned bytes,unsigned usage,unsigned format,unsigned pool,uintptr_t& output){check(d==device()&&bytes==((count+1u)&~1u)*2u&&usage==(8u|(dynamic?0x200u:0u)|(managed?0x10u:0u))&&format==0x65u&&pool==(dynamic?0u:1u));check(output==(creates==0?0u:(outputMask&1u?resourceBase+1:0u)));check(creates<2);createDevices[creates]=deviceIndex;++creates;if(outputMask&(1u<<(creates-1)))output=resourceBase+creates;if(mutateMetadata){check(state!=nullptr);state->fields[0]=7;state->fields[2]^=4u;}return creates==1?result:secondResult;}
 void cleanup(uintptr_t d){check(d==device());cleanupDevice=deviceIndex;++cleanups;}
 void logFailure(unsigned value){check(value==result);++logs;}
 unsigned lazyFlag=1,lazyCalls=0,registrations=0,callbackExecutions=0;void ensureAllocationCounter(){if(!(lazyFlag&1u)){lazyFlag|=1u;++lazyCalls;allocationCounter=studioFindProfileCounterCandidate(profile,counterName,2,fallback,*this);check(allocationCounter==(wrongThread||fullProfile?&fallback:&profile.values[0]));++registrations;auto before=counter;studioIndexAllocationCounterExitCandidate();++callbackExecutions;check(counter==before);if(mutateRegistration){check(state!=nullptr);state->fields[0]=9;}}}
 void addAllocationBytes(unsigned bytes){check(allocationCounter==&profile.values[0]||allocationCounter==&fallback);*allocationCounter+=bytes;counter=*allocationCounter;}
};
int main(){static_assert(sizeof(void*)==8,"x64 comparison");std::ifstream records("outputs/native-index-buffer-creation.bin",std::ios::binary);unsigned cases=0;
 for(unsigned count:{0u,1u,3u,0xffffffffu})for(unsigned dynamic:{0u,1u})for(unsigned managed:{0u,1u})for(unsigned result:{0u,1u,0x80004005u,0x8876017cu,0x8007000eu})for(unsigned retryResult:{0u,1u,0x80004005u,0x8876017cu,0x8007000eu})for(unsigned writes=0;writes<4;++writes)for(bool mutation:{false,true})for(bool rebind:{false,true})for(unsigned initialized:{0u,1u})for(bool wrongThread:{false,true})for(unsigned profileCount:{1u,255u,0u})for(bool registrationMutation:{false,true}){bool full=profileCount==255;
  StudioIndexBufferCreationCandidate object;object.resource=0xabababab;object.fields.fill(0xabababab);Ops ops{0x1234,count,dynamic,managed,result,retryResult,writes};ops.prepareCounter();ops.mutateRegistration=registrationMutation;ops.wrongThread=wrongThread;ops.fullProfile=full;ops.profile.count=profileCount;ops.lazyFlag=initialized;ops.rebindDevice=rebind;ops.mutateMetadata=mutation;ops.state=&object;studioIndexBufferCreationCandidate(object,count,managed,dynamic,ops);
  bool inserted=ops.lazyCalls&&profileCount==0&&!wrongThread;check(ops.allocations==unsigned(inserted));if(inserted)check(!std::strcmp(ops.nameStorage.data(),ops.counterName));
  check(ops.profileThreadCalls==(full?0u:ops.lazyCalls)&&ops.profileCompares==(wrongThread||full||profileCount==0?0u:ops.lazyCalls)&&ops.profile.count==profileCount+unsigned(inserted)&&ops.profile.names[0]==(inserted?ops.nameStorage.data():ops.counterName)&&ops.profile.groups[0]==2&&ops.profile.values[0]==((wrongThread||full)&&ops.lazyCalls?7u:ops.counter)&&ops.fallback==((wrongThread||full)&&ops.lazyCalls?ops.counter:99u));std::array<unsigned,21> actual{},original{};check(object.resource<=UINT32_MAX);actual[0]=static_cast<unsigned>(object.resource);std::copy(object.fields.begin(),object.fields.end(),actual.begin()+1);actual[8]=ops.creates;actual[9]=ops.flushes;actual[10]=ops.cleanups;actual[11]=ops.logs;actual[12]=ops.counter;actual[13]=ops.lazyFlag;actual[14]=ops.createDevices[0];actual[15]=ops.createDevices[1];actual[16]=ops.cleanupDevice;actual[17]=ops.deviceIndex;actual[18]=ops.lazyCalls;actual[19]=ops.registrations;actual[20]=ops.callbackExecutions;records.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(bool(records)&&actual==original);++cases;
 }
 check(records.peek()==std::char_traits<char>::eof());
 for(unsigned writes=0;writes<4;++writes){StudioIndexBufferCreationCandidate object;object.fields.fill(0xabababab);Ops ops{uintptr_t{0x100001234ull},3,0,0,0x8876017cu,0x80004005u,writes};ops.prepareCounter();studioIndexBufferCreationCandidate(object,3,0,0,ops);uintptr_t expected=writes&2u?ops.resourceBase+2:writes&1u?ops.resourceBase+1:0u;check(object.resource==expected&&ops.creates==2&&ops.logs==1);}
 std::printf("x64_full_width_resource_retention_cases=4 passed\n");std::printf("x64_index_buffer_creation_records_matched=%u passed\n",cases);
}
