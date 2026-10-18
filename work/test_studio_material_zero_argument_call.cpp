#include <array>
#include <iostream>
#include "studio_material_zero_argument_call_candidate.h"
static unsigned invoked;
static void method(uintptr_t owner){if(owner==0x100000001ULL)++invoked;}
int main(){std::array<StudioMaterialZeroArgumentCallCandidate,4096> arena;unsigned destroyed=0;
for(auto& call:arena){call.method=method;call.owner=0x100000001ULL;if(call.addReference()!=2)return 1;
auto destroy=[&](auto* object,unsigned flags){if(flags||object!=&call)throw 1;++destroyed;};
if(call.releaseReference(destroy)!=1||destroyed!=invoked)return 1;call.invoke();if(call.releaseReference(destroy)!=0||call.references||call.owner!=0x100000001ULL||call.method!=method)return 1;
}
if(invoked!=4096||destroyed!=4096)return 1;
std::cout<<"arena_functors=4096 invocations=4096 zero_flag_destructions=4096 heap_frees=0\n";}
