#include "reference_handle_retirement.h"
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8);if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("missing oracle");unsigned cases=0;
 for(unsigned index=0;index<8;++index)for(unsigned gen=0;gen<514;++gen)for(unsigned retired=0;retired<2;++retired){uint32_t generation=gen<512?gen:gen==512?0x7ffffffeu:0x7fffffffu;std::vector<RecoveredReferenceHandleSlotCandidate>slots(5,{generation|(retired<<31),0x1234});uint32_t active=9,calls=0,appended=0xffffffffu;retireRecoveredReferenceHandleCandidate(0x7c000000u|index,slots,active,[&](uint32_t i){++calls;appended=i;});std::array<uint32_t,13>actual{},expected{};actual[0]=active;for(unsigned i=0;i<5;++i){actual[1+i*2]=slots[i].state;actual[2+i*2]=uint32_t(slots[i].element);}actual[11]=calls;actual[12]=appended;f.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!f||actual!=expected)throw std::runtime_error("native retirement mismatch");++cases;}
 if(f.peek()!=std::char_traits<char>::eof())throw std::runtime_error("trailing oracle");std::cout<<"native_reference_handle_retirement_matches="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
