#include <string>
#include "reverse_reference_acceptance.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);unsigned cases=0;for(unsigned kind=0;kind<2;++kind)for(unsigned mode=0;mode<5;++mode)for(unsigned flags=0;flags<4;++flags)for(unsigned bits=0;bits<128;++bits){uint32_t actual=recoveredReverseReferenceAcceptedCandidate(bits&1,bits&2,bits&4,uint16_t(((flags&1)?0x800:0)|((flags&2)?0x1000:0)),9,(bits&64)?0xffffffffu:((bits&8)?9:10),bits&16,bits&32,mode);uint32_t expected;f.read(reinterpret_cast<char*>(&expected),4);if(!f||actual!=expected)throw std::runtime_error("predicate mismatch at "+std::to_string(cases));++cases;}char extra;if(f.read(&extra,1))throw std::runtime_error("trailing oracle");std::cout<<"native_reference_predicate_matches="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}

