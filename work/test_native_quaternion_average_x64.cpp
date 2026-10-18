#include "quaternion_log_average.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdint>
int main(int argc,char**argv){static_assert(sizeof(void*)==8,"x64 required");
    if(argc!=2)return 2;std::ifstream in(argv[1],std::ios::binary);if(!in)return 2;
    uint32_t count,pattern;unsigned checks=0,mismatches=0;
    while(in.read(reinterpret_cast<char*>(&count),4)){
        std::array<float,4> expected{};in.read(reinterpret_cast<char*>(&pattern),4);in.read(reinterpret_cast<char*>(expected.data()),16);
        if(!in||count>5||pattern>=256)return 2;
        std::vector<std::array<float,4>>q(count);std::vector<float>w(count);
        for(unsigned i=0;i<count;++i){for(unsigned c=0;c<4;++c)q[i][c]=float(int((pattern*17+i*19+c*13)%127)-63)*.03125f;w[i]=float(int((pattern+i*3)%9)-3)*.25f;}
        auto actual=recoveredQuaternionLogAverageCandidate(q,w);
        if(std::memcmp(actual.data(),expected.data(),16)){std::cout<<"mismatch count="<<count<<" pattern="<<pattern;for(unsigned c=0;c<4;++c){uint32_t a,e;std::memcpy(&a,&actual[c],4);std::memcpy(&e,&expected[c],4);std::cout<<" component"<<c<<"="<<std::hex<<a<<"/"<<e<<std::dec;}std::cout<<std::endl;++mismatches;}++checks;
    }
    std::cout<<"quaternion_average_checks="<<checks<<" mismatches="<<mismatches<<std::endl;
    return checks==1536&&!mismatches?0:1;
}
