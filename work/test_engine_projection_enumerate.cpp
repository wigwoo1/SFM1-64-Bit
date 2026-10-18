#include "engine_projection_enumerate_candidate.h"
#include <cstdio>
#include <cstdlib>
void check(bool v){if(!v)std::abort();}
int main(){unsigned cases=0;
 for(int count=-2;count<=64;++count){EngineProjectionPayloadCandidate payload;std::memset(&payload,0xa5,sizeof(payload));uint32_t generation=0xffffffff;
  uint32_t a[64],b[64];for(int i=0;i<64;++i){a[i]=uint32_t(i+100);b[i]=uint32_t(i+1000);}const uint32_t* current=a;unsigned setups=0,reads=0,reloads=0,visits=0;
  engineProjectionEnumerateCandidate(count,0x8123,generation,payload,[&]{++setups;},[&](float* projection,float* center,float& radius){check(setups==1);++reads;for(int k=0;k<3;++k){projection[k]=float(k+2);center[k]=float(k+7);}radius=11;},[&]{++reloads;return current;},[&](uint32_t item,EngineProjectionPayloadCandidate& p){check(item==(visits==0?100:visits+1000));check(&p==&payload);current=b;++visits;});
  check(setups==unsigned(count!=0)&&reads==setups);check(generation==(count==0?0xffffffff:0));check(visits==unsigned(count>0?count:0)&&reloads==visits);
  for(int i=0;i<48;++i)if(count==0||(i>=2&&i<4)||(i>=16&&i<28))check(payload.bytes[i]==0xa5);
  if(count!=0){uint16_t handle;std::memcpy(&handle,payload.bytes,2);check(handle==0x8123);float center[3],radius;std::memcpy(center,payload.bytes+28,12);std::memcpy(&radius,payload.bytes+40,4);check(center[0]==7&&center[1]==8&&center[2]==9&&radius==11);uint32_t filter;std::memcpy(&filter,payload.bytes+44,4);check(filter==0);}++cases;
 }
 std::printf("PASS: %u counts; setup/generation wrap, negative setup, preserved payload padding and callback array replacement. Native caller integration pending.\n",cases);
}
