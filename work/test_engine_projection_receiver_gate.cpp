#include "engine_projection_receiver_gate_candidate.h"
#include "engine_projection_generate_candidate.h"
#include <cstdio>
#include <cstdlib>
void check(bool ok){if(!ok)std::abort();}
int main(){float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},center[3]={.5f,.5f,.5f};EngineProjectionPlanesCandidate p;engineProjectionGeneratePlanesCandidate(identity,p);unsigned cases=0;
 for(unsigned special=0;special<2;++special)for(unsigned mask=0;mask<8;++mask)for(unsigned eligible=0;eligible<2;++eligible)for(unsigned shadow=0;shadow<8;++shadow)for(unsigned outside=0;outside<2;++outside){
  uint32_t flags=(mask&1?0x20:0)|(mask&2?0x1000:0)|(mask&4?0x2000:0);EngineProjectionObjectRecordCandidate record{flags,uint8_t(eligible)};unsigned reads=0,bounds=0,planes=0,shadowReads=0,associations=0;
  bool actual=engineProjectionReceiverGateCandidate(special?0x101:0x100,0x8123,center,1,[&]{++reads;return &record;},[&](float* low,float* high){++bounds;for(int k=0;k<3;++k){low[k]=outside?3.f:.25f;high[k]=outside?4.f:.75f;}},[&](uint16_t h)->const EngineProjectionPlanesCandidate&{check(h==0x8123);++planes;return p;},[&](uint16_t h){check(h==0x8123);++shadowReads;return shadow;},[&](auto* r,uint16_t h){check(r==&record&&h==0x8123);++associations;});
  bool prefilter=!special&&(flags&0x2000);bool spatial=!outside;bool expected=!prefilter&&spatial&&(special||eligible)&&((shadow&6)||!flags);
  check(actual==expected&&associations==unsigned(expected));check(bounds==unsigned(!prefilter));check(planes==unsigned(special));check(reads==unsigned(!special)+unsigned(!prefilter&&spatial));check(shadowReads==unsigned(!prefilter&&spatial&&(special||eligible)));++cases;
 }
 EngineProjectionObjectRecordCandidate first{0,0},second{0,1};auto* current=&first;unsigned reads=0,associated=0;
 check(engineProjectionReceiverGateCandidate(0,7,center,1,[&]{++reads;return current;},[&](float* low,float* high){for(int k=0;k<3;++k){low[k]=0;high[k]=1;}current=&second;},[&](uint16_t)->const EngineProjectionPlanesCandidate&{return p;},[](uint16_t){return 0;},[&](auto* r,uint16_t){check(r==&second);++associated;}));check(reads==2&&associated==1);
 std::printf("PASS: %u integrated gate combinations plus callback record replacement; native full-gate parity pending.\n",cases);
}
