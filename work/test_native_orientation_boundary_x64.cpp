#include "transform_orientation.h"
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
        if(!input || pattern>=257) return 2;
        uint32_t bits=uint32_t(int64_t(0x3f800000)+int(pattern)-128);float scale;std::memcpy(&scale,&bits,4);
        if(pattern<256){a[0][0]=scale;a[1][1]=1;a[2][2]=1;a[0][1]=.00001f;a[1][0]=-.00001f;}
        auto actual=extractRecoveredTransformOrientationCandidate(a);
        if(std::memcmp(actual.data(),expected.data(),16)){std::cout<<"mismatch pattern="<<pattern<<std::endl;++mismatches;}
        ++checks;
    }
    std::cout<<"transform_orientation_checks="<<checks<<" mismatches="<<mismatches<<'\n';
    return checks==257 && !mismatches ? 0 : 1;
}
