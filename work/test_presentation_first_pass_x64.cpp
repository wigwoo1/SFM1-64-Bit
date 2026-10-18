#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include "studio_presentation_first_pass_candidate.h"
struct Services{
 int32_t width,height;unsigned step=0,sets=0,gets=0;std::array<unsigned,3> values{};bool valid=true;
 void displayMode(std::array<uint32_t,6>& value,unsigned adapter){valid&=++step==1&&adapter==3&&value[0]==1;value.fill(0);}
 void dimensions(std::array<uint32_t,6>& value,unsigned&,int32_t& w,int32_t& h){valid&=++step==2;for(auto word:value)valid&=word==0;w=width;h=height;}
 float aspectThreshold(){return 1.5f;}
 Services& material(){++gets;return *this;}
 void setCondition(const char* name,int value){const char* names[]={"WIN32WIDE","WIN32HIDEF","WIN32LODEF"};if(sets>=3){valid=false;return;}valid&=!std::strcmp(name,names[sets]);values[sets++]=static_cast<unsigned>(value);}
};
int main(){std::ifstream oracle("outputs/native-presentation-first-pass.bin",std::ios::binary);unsigned cases=0;
 for(int w:{640,1080,1920})for(int h:{480,719,720,1080})for(unsigned pattern:{0u,0x55u,0xffu}){
 unsigned descriptor=pattern;Services services{w,h};studioPresentationFirstPassCandidate(3,descriptor,services);
 std::array<unsigned,6> expected{},record{services.step,services.sets,services.gets,services.values[0],services.values[1],services.values[2]};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!oracle||!services.valid||record!=expected||descriptor!=pattern)return 1;++cases;
 }if(oracle.peek()!=std::char_traits<char>::eof())return 2;std::printf("x64_presentation_first_pass_native_matches=%u gpu_executed=0\n",cases);}
