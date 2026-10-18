#include "controller_container_lookup_candidate.h"
#include <cassert>
#include <iostream>
struct Node{void* data;};struct Adapter{int payload=7;Node node{&payload};Node* slot=&node;unsigned error=9,hits=0,misses=0;bool mutate=false;
 void setLookupError(unsigned n){error=n;}Node** findNodeSlot(const unsigned* key,unsigned* output){assert(!error && *key==42);*output=13;if(mutate)hits=0xffffffff;return &slot;}
 void* nodePayload(Node* p){return p->data;}void* nullPayload(){return nullptr;}unsigned hitCount(){return hits;}void setHitCount(unsigned n){hits=n;}unsigned missCount(){return misses;}void setMissCount(unsigned n){misses=n;}
};
int main(){unsigned key=42;Adapter a;assert(lookupControllerContainerCandidate(&key,a)==&a.payload && a.hits==1 && !a.misses && !a.error);Adapter b;b.slot=nullptr;b.misses=0xffffffff;assert(!lookupControllerContainerCandidate(&key,b) && !b.misses && !b.hits);Adapter c;c.node.data=nullptr;assert(!lookupControllerContainerCandidate(&key,c) && c.hits==1);Adapter d;d.mutate=true;lookupControllerContainerCandidate(&key,d);assert(!d.hits);
 std::cout<<"x64 container lookup: cleared error, slot hit/miss, null payload hit and live counter wrap passed\n";
}
