#include "point_constraint_blend.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdint>
int main(int argc,char**argv){static_assert(sizeof(void*)==8,"x64 required");
    if(argc!=2)return 2;std::ifstream in(argv[1],std::ios::binary);if(!in)return 2;
    uint32_t pattern;unsigned checks=0,mismatches=0;
    while(in.read(reinterpret_cast<char*>(&pattern),4)){float influence;std::array<float,3>a{},s{},expected{};
        in.read(reinterpret_cast<char*>(&influence),4);in.read(reinterpret_cast<char*>(expected.data()),12);
        if(!in||pattern>=32)return 2;
        for(unsigned c=0;c<3;++c){a[c]=float(int(pattern*7+c*3)-55)*.125f;s[c]=float(int(pattern*11+c*5)-73)*.0625f;}
        auto actual=blendRecoveredPointConstraintPosition(a,influence,s);
        if(std::memcmp(actual.data(),expected.data(),12))++mismatches;++checks;
    }
    std::cout<<"point_blend_checks="<<checks<<" mismatches="<<mismatches<<'\n';return checks==9632&&!mismatches?0:1;
}
