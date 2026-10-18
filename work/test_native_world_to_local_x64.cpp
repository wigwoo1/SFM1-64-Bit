#include "transform_world_to_local.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdint>
int main(int argc,char**argv) {
    static_assert(sizeof(void*)==8,"x64 required");
    if(argc!=2) return 2;
    std::ifstream input(argv[1],std::ios::binary);
    if(!input) return 2;
    uint32_t pattern,shared;unsigned checks=0,mismatches=0;
    while(input.read(reinterpret_cast<char*>(&pattern),4)) {
        float ps,rs;RecoveredMatrix3x4 position{},rotation{},world{},expected{};
        input.read(reinterpret_cast<char*>(&shared),4);
        input.read(reinterpret_cast<char*>(&ps),4);
        input.read(reinterpret_cast<char*>(&rs),4);
        input.read(reinterpret_cast<char*>(expected.data()),48);
        if(!input || pattern>=1024 || shared>1) return 2;
        for(unsigned i=0;i<12;++i) {
            position[i/4][i%4]=float(int((pattern*17+i*13)%127)-63)*.03125f;
            rotation[i/4][i%4]=float(int((pattern*23+i*7)%113)-56)*.0625f;
            world[i/4][i%4]=float(int((pattern*19+i*11)%109)-54)*.125f;
        }
        auto actual=convertRecoveredWorldToLocal(world,position,ps,shared?position:rotation,rs,shared!=0);
        if(std::memcmp(actual.data(),expected.data(),48)) ++mismatches;
        ++checks;
    }
    std::cout<<"world_to_local_checks="<<checks<<" mismatches="<<mismatches<<'\n';
    return checks==2048 && !mismatches ? 0 : 1;
}
