#include "controller_shutdown_tail_candidate.h"
#include <cassert>
#include <vector>
#include <string>
#include <iostream>
struct Adapter{
 int first=1,second=2;int* pool=&first;unsigned refs=1,flags=3;bool mutate=false;std::vector<std::string> calls;
 int* threadPool(){return pool;}void invokePoolSlot5(int* p,unsigned n){assert(p==&first && n==0xffffffff);calls.push_back("slot5");if(mutate)pool=&second;}
 void destroyThreadPool(int* p){assert(p==(mutate?&second:&first));calls.push_back("destroy");}void clearThreadPool(){pool=nullptr;calls.push_back("clear");}
 unsigned referenceCount(){return refs;}void setReferenceCount(unsigned n){refs=n;}
 unsigned subsystemFlags(){return flags;}void setSubsystemFlags(unsigned n){flags=n;}
 void cleanup2c90(){calls.push_back("2c90");}void cleanup27f0(){calls.push_back("27f0");if(mutate)flags&=~2u;}void cleanup0d50(){calls.push_back("0d50");}
};
int main(){Adapter a;controllerShutdownTailCandidate(a);assert((a.calls==std::vector<std::string>{"slot5","destroy","clear","2c90","27f0","0d50"}) && !a.pool && !a.refs && !a.flags);
 Adapter b;b.mutate=true;controllerShutdownTailCandidate(b);assert(b.calls.size()==5 && !b.flags);
 Adapter c;c.pool=nullptr;c.refs=2;controllerShutdownTailCandidate(c);assert(c.calls.empty() && c.refs==1 && c.flags==3);
 Adapter d;d.pool=nullptr;d.refs=0;controllerShutdownTailCandidate(d);assert(d.calls.empty() && !d.refs);
 Adapter e;e.pool=nullptr;e.refs=0xffffffff;controllerShutdownTailCandidate(e);assert(e.refs==0xfffffffe && e.calls.empty());
 std::cout<<"x64 shutdown tail: pool ordering/live pointer, absent pool, refcount gates and live flags passed\n";
}
