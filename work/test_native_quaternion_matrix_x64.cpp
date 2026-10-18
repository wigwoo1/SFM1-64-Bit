#include "quaternion_transform_matrix.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdint>
int main(int argc,char**argv){
    static_assert(sizeof(void*)==8,"x64 required");
    if(argc!=2)return 2;
    std::ifstream input(argv[1],std::ios::binary);if(!input)return 2;
    unsigned checks=0,mismatches=0;uint32_t pattern;
    while(input.read(reinterpret_cast<char*>(&pattern),4)){
        RecoveredMatrix3x4 expected{};std::array<float,4>q{};std::array<float,3>p{};
        input.read(reinterpret_cast<char*>(expected.data()),48);
        if(!input||pattern>=4096)return 2;
        for(unsigned i=0;i<4;++i)q[i]=float(int((pattern*17+i*13)%127)-63)*.03125f;
        for(unsigned i=0;i<3;++i)p[i]=float(int((pattern*23+i*7)%113)-56)*.0625f;
        auto actual=recoveredQuaternionTransformMatrix(q,p);
        if(std::memcmp(actual.data(),expected.data(),48))++mismatches;
        ++checks;
    }
    std::cout<<"quaternion_matrix_checks="<<checks<<" mismatches="<<mismatches<<'\n';
    return checks==4096&&!mismatches?0:1;
}
