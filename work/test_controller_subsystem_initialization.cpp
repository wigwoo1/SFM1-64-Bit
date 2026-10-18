#include "controller_subsystem_initialization_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Adapter{unsigned refs=0,flags=77;bool first=true;unsigned second=0;std::vector<unsigned> calls;
 unsigned referenceCount(){return refs;}void setReferenceCount(unsigned n){refs=n;}void installAllocatorHooks(){calls.push_back(1);}
 bool initialize27d0(){calls.push_back(2);return first;}unsigned initialize0d60(){calls.push_back(3);return second;}
 void setSubsystemFlags(unsigned n){flags=n;}void initialize25e0(){calls.push_back(4);}
};
int main(){for(unsigned flags=0;flags<4;++flags){Adapter a;assert(initializeControllerSubsystemCandidate(flags,a)==0 && a.refs==1 && a.flags==flags && a.calls.front()==1 && a.calls.back()==4);auto n=a.calls.size();assert(initializeControllerSubsystemCandidate(99,a)==0 && a.refs==2 && a.calls.size()==n && a.flags==flags);}
 Adapter b;b.first=false;assert(initializeControllerSubsystemCandidate(3,b)==2 && b.refs==1 && b.flags==77 && b.calls.size()==2);
 Adapter c;c.second=1;assert(initializeControllerSubsystemCandidate(3,c)==2 && c.refs==1 && c.flags==77 && c.calls.size()==3);
 Adapter d;d.refs=0xffffffff;assert(initializeControllerSubsystemCandidate(3,d)==0 && d.refs==0 && d.calls.empty());
 std::cout<<"x64 subsystem initialization: flag gates, nested reference, failure without rollback and raw32 wrap passed\n";
}
