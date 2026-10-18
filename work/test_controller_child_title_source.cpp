#include "controller_child_title_source_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Owner{std::uint32_t value;};
struct Adapter {
    Owner first{7},second{9};int resolves=0;bool missing=false;std::vector<std::uint32_t> handles;
    std::uint32_t childHandle80(){return resolves?22:11;}
    Owner* resolveOwner(std::uint32_t h){handles.push_back(h);++resolves;return missing?nullptr:(resolves==1?&first:&second);}
    std::uint32_t ownerField38(Owner* p){return p->value;}
    std::uint32_t modelSlot97(std::uint32_t id){return id;}
};
int main(){Adapter a;assert(controllerChildTitleSourceCandidate(a)==9);assert((a.handles==std::vector<std::uint32_t>{11,22}));
    Adapter b;b.missing=true;assert(controllerChildTitleSourceCandidate(b)==0xffffffff && b.resolves==1);
    std::cout<<"x64 child title source: double resolution/live handle reload/second owner field/missing sentinel passed\n";
}
