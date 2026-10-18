#include "matrix_quaternion.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdint>
int main(int argc, char**argv) {
    static_assert(sizeof(void*) == 8, "x64 required");
    if (argc != 2) return 2;
    std::ifstream input(argv[1], std::ios::binary);
    if (!input) return 2;
    uint32_t pattern;
    unsigned checks=0, mismatches=0;
    while(input.read(reinterpret_cast<char*>(&pattern),4)) {
        RecoveredMatrix3x4 a{},b{};std::array<float,4> expected{};
        input.read(reinterpret_cast<char*>(expected.data()),16);
        if(!input || pattern>=2048) return 2;
        for(unsigned i=0;i<12;++i) {
            a[i/4][i%4]=float(int((pattern*17+i*13)%127)-63)*.03125f;
            b[i/4][i%4]=float(int((pattern*23+i*7)%113)-56)*.0625f;
        }
        auto actual=extractRecoveredMatrixQuaternion(a);
        if(std::memcmp(actual.data(),expected.data(),16)) ++mismatches;
        ++checks;
    }
    std::cout<<"matrix_quaternion_checks="<<checks<<" mismatches="<<mismatches<<'\n';
    return checks==2048 && !mismatches ? 0 : 1;
}
