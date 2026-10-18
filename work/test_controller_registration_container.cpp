#include "controller_registration_container_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Adapter{int first=1,second=2;int* cache=nullptr;bool replace=false;unsigned allocations=0;std::vector<unsigned> events;
 void guardService(unsigned action,unsigned kind,unsigned line){assert(kind==1 && line==(action==9?263u:272u));events.push_back(action);if(action==10 && replace)cache=&first;}
 int* cachedContainer(){return cache;}void setCachedContainer(int* p){cache=p;}
 void beginContainerAllocation(unsigned line){assert(line==266);events.push_back(1);}int* allocateContainer(unsigned hash,unsigned compare){assert(hash==0x10431320 && compare==0x1043cfa0);++allocations;return &first;}
 void endContainerAllocation(){events.push_back(2);if(replace)cache=&second;}
};
int main(){Adapter a;assert(!getControllerRegistrationContainerCandidate(0,a) && !a.allocations && (a.events==std::vector<unsigned>{9,10}));assert(getControllerRegistrationContainerCandidate(1,a)==&a.first && a.allocations==1);getControllerRegistrationContainerCandidate(1,a);assert(a.allocations==1);
 Adapter b;b.replace=true;assert(getControllerRegistrationContainerCandidate(1,b)==&b.second && b.cache==&b.first);
 std::cout<<"x64 container getter: create gate, cached bypass, post-allocation reload and saved result passed\n";
}
