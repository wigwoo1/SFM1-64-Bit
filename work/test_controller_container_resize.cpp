#include "controller_container_resize_candidate.h"
#include <cassert>
#include <iostream>
struct Node{Node* next;unsigned hash;};struct Adapter{Node* storage[32]={};unsigned split=0,mod=8,cap=16,active=8,splitCalls=0,doubleCalls=0,error=0;bool fail=false;
 unsigned split14(){return split;}void setSplit14(unsigned n){split=n;}unsigned divisor18(){return mod;}void setDivisor18(unsigned n){mod=n;}unsigned divisor10(){return cap;}void setDivisor10(unsigned n){cap=n;}
 Node** reallocateBuckets(unsigned n){assert(n==32);return fail?nullptr:storage;}void setBucketStorage(Node** p){assert(p==storage);}Node** bucketSlot(unsigned n){assert(n<32);return &storage[n];}
 unsigned lookupError(){return error;}void setLookupError(unsigned n){error=n;}unsigned doublings(){return doubleCalls;}void setDoublings(unsigned n){doubleCalls=n;}unsigned splits(){return splitCalls;}void setSplits(unsigned n){splitCalls=n;}unsigned activeBuckets(){return active;}void setActiveBuckets(unsigned n){active=n;}
};
int main(){Node d{nullptr,24},c{&d,16},b{&c,8},a{&b,0};Adapter first;first.storage[0]=&a;resizeControllerContainerCandidate(first);assert(first.storage[0]==&a && a.next==&c && !c.next && first.storage[8]==&d && d.next==&b && !b.next && first.split==1 && first.active==9);
 Node y{nullptr,15},x{&y,7};Adapter wrap;wrap.split=7;wrap.active=15;wrap.storage[7]=&x;resizeControllerContainerCandidate(wrap);assert(wrap.cap==32 && wrap.mod==16 && !wrap.split && wrap.active==16 && wrap.doubleCalls==1 && wrap.storage[7]==&x && !x.next && wrap.storage[15]==&y);
 Adapter failure;failure.split=7;failure.fail=true;resizeControllerContainerCandidate(failure);assert(!failure.split && failure.error==1 && failure.active==8 && !failure.splitCalls && failure.cap==16);
 std::cout<<"x64 incremental resize: stay/move order, wrap doubling, counts and failed allocation state passed\n";
}
