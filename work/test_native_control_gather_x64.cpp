#include "ordered_control_channel_gather.h"
#include <fstream>
#include <iostream>
#include <array>
#include <stdexcept>
int main(int argc,char**argv){try{if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);unsigned cases=0;
for(unsigned count=0;count<=4;++count)for(unsigned mask=0;mask<(1u<<count);++mask)for(unsigned variant=0;variant<4;++variant)for(unsigned present=0;present<2;++present){std::vector<uint32_t>slaves,calls;for(unsigned i=0;i<count;++i)slaves.push_back((mask&(1u<<i))?0xffffffffu:20+(i+variant)%3);
auto result=gatherRecoveredControlChannelsCandidate(1,present?10:0xffffffffu,[&](std::vector<uint32_t>&out,uint32_t source){calls.push_back(source==0xffffffffu?0:source);if(source!=0xffffffffu){out.push_back(source%3+100);out.push_back(source%3+100);}},[&](uint32_t){return slaves;});std::array<uint32_t,32>actual{},expected{};actual[0]=uint32_t(result.size());actual[1]=uint32_t(calls.size());for(unsigned i=0;i<result.size();++i)actual[2+i]=result[i];for(unsigned i=0;i<calls.size();++i)actual[18+i]=calls[i];f.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!f||actual!=expected)throw std::runtime_error("native gather order mismatch");++cases;}
char extra;if(f.read(&extra,1))throw std::runtime_error("trailing oracle");std::cout<<"native_control_gather_matches="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
