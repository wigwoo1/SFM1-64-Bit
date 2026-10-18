#include "controller_startup_generator_candidate.h"
#include <cassert>
#include <iostream>
struct Adapter{unsigned state=0,seed=0,calls=0;unsigned randomState(){return state;}void setRandomState(unsigned v){state=v;}unsigned seedHelper(unsigned n){assert(n==0);++calls;return seed;}};
int main(){unsigned cases[][3]={{0x0u,0xa70427dfu,0x27dfa704u},{0x1u,0x2781e494u,0xe4942781u},{0x7fffffffu,0xa6866b2au,0x6b2aa686u},{0x80000000u,0x270427dfu,0x27df2704u},{0xffffffffu,0x26866b2au,0x6b2a2686u},{0x75bcd15u,0xe8c893b8u,0x93b8e8c8u}};
 for(auto& row:cases){Adapter a;a.seed=row[0];assert(seedControllerStartupRandomCandidate(a)==row[2] && a.state==row[1] && a.calls==1);}
 Adapter b;assert(nextControllerStartupRandomCandidate(b)==0x30390000u && b.state==0x3039u && !b.calls);
 std::cout<<"x64 startup generator: fixed wide-arithmetic reference vectors, unrotated state and three-step warmup passed\n";
}
