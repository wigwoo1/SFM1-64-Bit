#include "controller_container_allocation_candidate.h"
#include <cassert>
#include <iostream>
struct Adapter{unsigned fail=0,objects=0,buckets=0,releases=0;
 ControllerContainerLayoutCandidate* allocateObject(){++objects;return fail==1?nullptr:new ControllerContainerLayoutCandidate;}
 void** allocateBuckets(unsigned n){assert(n==16);++buckets;return fail==2?nullptr:new void*[n];}
 void releaseObject(ControllerContainerLayoutCandidate* p){++releases;delete p;}
};
int main(){static_assert(sizeof(void*)==8);Adapter a;auto* p=allocateControllerContainerCandidate(0x100000001ULL,0x100000002ULL,a);assert(p && p->hashTag==0x100000001ULL && p->compareTag==0x100000002ULL && p->fields[0]==8 && p->fields[1]==16 && p->fields[3]==8 && p->fields[4]==512 && p->fields[5]==256);for(unsigned i=0;i<16;++i)assert(!p->buckets[i]);for(unsigned i=6;i<21;++i)assert(!p->fields[i]);delete[] p->buckets;delete p;
 Adapter b;b.fail=1;assert(!allocateControllerContainerCandidate(0,0,b) && !b.buckets && !b.releases);
 Adapter c;c.fail=2;assert(!allocateControllerContainerCandidate(0,0,c) && c.releases==1);
 Adapter d;p=allocateControllerContainerCandidate(0,0,d);assert(p->hashTag==0x10442c00 && p->compareTag==0x10a182f0);delete[] p->buckets;delete p;
 std::cout<<"x64 container allocation: native bucket widths, initial fields, callback tags and both failure paths passed\n";
}
