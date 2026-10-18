#include "native_srgb_sampler_state_candidate.h"
#include <array>
#include <iostream>
int main(){try{
 unsigned cases=0;
 for(unsigned initial=0;initial<256;++initial)for(unsigned enabled=0;enabled<256;++enabled)for(bool capability:{false,true}){
  std::array<uint8_t,3>flags{0x55,uint8_t(initial),0xaa};
  setNativeSrgbSamplerStateCandidate(flags.data(),flags.size(),1,uint8_t(enabled),capability,capability?3:-1);
  unsigned expected=(initial&0xfdu)|(capability?((enabled&1u)<<1):0);
  if(flags[1]!=expected||flags[0]!=0x55||flags[2]!=0xaa)throw std::runtime_error("sampler state or adjacent bytes changed");
  ++cases;
 }
 std::array<uint8_t,3>flags{0x55,0xff,0xaa};unsigned rejected=0;
 auto reject=[&](int sampler,bool capability,int count){auto before=flags;try{setNativeSrgbSamplerStateCandidate(flags.data(),flags.size(),sampler,1,capability,count);}catch(const std::runtime_error&){++rejected;}if(flags!=before)throw std::runtime_error("invalid sampler mutated state");};
 reject(-1,false,3);reject(3,false,3);reject(1,true,1);reject(1,true,-1);
 try{setNativeSrgbSamplerStateCandidate(nullptr,3,0,1,true,3);}catch(const std::runtime_error&){++rejected;}
 if(rejected!=5)throw std::runtime_error("invalid sampler accepted");
 std::cout<<"PASS x64 native SRGBRead state: "<<cases<<" bit/capability cases, adjacent bytes preserved, five invalid inputs rejected without mutation\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
