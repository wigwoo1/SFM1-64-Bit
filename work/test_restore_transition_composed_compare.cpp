#define main priorCompositionMain
#include "test_restore_blend_transition_composition.cpp"
#undef main
#include <array>
#include <fstream>
#include <cstring>
struct CompareServices:Services{
 using Services::Services;bool mutateDescriptor=false;unsigned suppressed(){if(mutateDescriptor)parent.descriptor=0;return suppression;}void sampler(unsigned stage,unsigned key,unsigned value){Services::sampler(stage,key,value);if(direct&&stage==0&&key==11){stages=0;parent.samplers[1].first=23;}}bool direct=false;void render(unsigned key,unsigned value){Services::render(key,value);if(direct&&key==27)parent.f100=99;}unsigned special=0;unsigned specialSamplerEnabled(){return special;}int stages=0;int samplerCount(){return stages;}
 void transition(int kind,unsigned offset,unsigned count){if(count==0){++transitions;return;}Services::transition(kind,offset,count);}
};
int main(){std::ifstream oracle("outputs/native-restore-transition-composed.bin",std::ios::binary);unsigned cases=0;
 for(unsigned change:{0u,1u})for(unsigned mode:{0u,1u})for(unsigned special:{0u,1u,256u})for(unsigned stages:{0u,1u,2u})for(unsigned nonempty:{0u,1u})for(unsigned gate:{0u,1u,256u}){
  StudioRestoreBlendStateCandidate parent;parent.cachedKind=9;parent.cachedHandle=12;parent.descriptor=nonempty?0x04000001:0;parent.commit.dirty134=parent.commit.dirty135=parent.commit.dirty13c=parent.commit.dirty13e=parent.f144=1;
  StudioTransitionState72Candidate model{};model.flags44=255;model.flags45=1;model.values[0]=3;model.values[1]=1;model.values[4]=model.values[7]=model.values[10]=7;model.values[5]=model.values[8]=model.values[11]=13;model.values[6]=model.values[9]=17;
  parent.samplers={{17,0,0},{19,1,0}};CompareServices services(parent,model,gate);services.mutateDescriptor=change!=0;services.direct=mode==0;services.special=special;services.stages=static_cast<int>(stages);studioRestoreBlendCandidate(parent,services);
  std::array<unsigned,280> record{},native{};auto* bytes=reinterpret_cast<unsigned char*>(record.data());record[8/4]=parent.descriptor;std::memcpy(bytes+0x54,&parent.cachedKind,2);std::memcpy(bytes+0x56,&parent.cachedHandle,2);
  bytes[0x145]=17;bytes[0x148]=parent.samplers[1].first;bytes[0x149]=1;bytes[0xfc]=parent.ffc;bytes[0x10c]=parent.f10c;bytes[0x128]=parent.f128;bytes[0x144]=parent.f144;
  const unsigned fields[]{parent.f100,parent.f104,parent.f108,parent.f110,parent.f114,parent.f118,parent.f11c,parent.f120,parent.f124,parent.f12c,parent.f130};const unsigned offsets[]{0x100,0x104,0x108,0x110,0x114,0x118,0x11c,0x120,0x124,0x12c,0x130};for(unsigned i=0;i<11;++i)record[offsets[i]/4]=fields[i];
  for(unsigned i=128;i<256;++i)record[i]=0xcccccccc;unsigned used=0;for(unsigned i=0;i<services.commands.size();i+=2){record[128+used++]=1;record[128+used++]=services.commands[i];record[128+used++]=services.commands[i+1];if(i==24)for(unsigned sample=0;sample<services.samplers.size();sample+=3){record[128+used++]=17;record[128+used++]=services.samplers[sample];record[128+used++]=services.samplers[sample+1];record[128+used++]=services.samplers[sample+2];}}
  std::memcpy(record.data()+256,&model,72);record[274]=static_cast<unsigned>(services.samplers.size()/3)+static_cast<unsigned>(services.commands.size()/2);record[275]=record[276]=1;record[277]=services.shaderCalls;record[278]=services.depthCalls;record[279]=mode?128-used:128;
  oracle.read(reinterpret_cast<char*>(native.data()),sizeof(native));if(!oracle)return 1;for(unsigned i=0;i<280;++i)if(record[i]!=native[i]){std::printf("parent mismatch case=%u word=%u actual=%u native=%u\n",cases,i,record[i],native[i]);return 2;}++cases;
 }
 if(oracle.peek()!=std::char_traits<char>::eof())return 3;std::printf("restore_transition_composed_x64_cases=%u passed\n",cases);
}






