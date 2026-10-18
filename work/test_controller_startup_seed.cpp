#include "controller_startup_seed_candidate.h"
#include <cassert>
#include <iostream>
int main(){constexpr std::uint64_t epoch=116444736000000000ULL,limit=0x793406fffULL;std::uint64_t out=99;
 assert(controllerStartupSeedCandidate(epoch,&out)==0 && out==0);
 assert(controllerStartupSeedCandidate(epoch+9999999)==0);
 assert(controllerStartupSeedCandidate(epoch+10000000)==1);
 assert(controllerStartupSeedCandidate(epoch-1)==~std::uint64_t{0});
 assert(controllerStartupSeedCandidate(epoch+limit*10000000)==limit);
 assert(controllerStartupSeedCandidate(epoch+limit*10000000+9999999)==limit);
 assert(controllerStartupSeedCandidate(epoch+(limit+1)*10000000,&out)==~std::uint64_t{0} && out==~std::uint64_t{0});
 std::cout<<"x64 startup seed: epoch, second boundary, pre-epoch wrap, upper limit and optional output passed\n";
}
