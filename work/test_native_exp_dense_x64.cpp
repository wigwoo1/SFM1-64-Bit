#include "legacy_exp_finite.h"
#include <fstream>
#include <iostream>
int main(int argc,char**argv){static_assert(sizeof(void*)==8,"x64 required");
    if(argc!=2)return 2;std::ifstream in(argv[1],std::ios::binary);if(!in)return 2;
    float input,expected;unsigned checks=0,mismatches=0,rejected=0;
    while(in.read(reinterpret_cast<char*>(&input),4)){
        in.read(reinterpret_cast<char*>(&expected),4);if(!in)return 2;
        try{float actual=recoveredLegacyExpFinite(input);if(std::memcmp(&actual,&expected,4)){if(mismatches<4)std::cout<<"mismatch input="<<input<<std::endl;++mismatches;}}
        catch(const std::exception&){++rejected;}++checks;
    }
    std::cout<<"exp_checks="<<checks<<" mismatches="<<mismatches<<" rejected="<<rejected<<std::endl;
    return checks==174001&&!mismatches&&!rejected?0:1;
}
