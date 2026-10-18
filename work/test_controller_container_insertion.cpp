#include "controller_container_insertion_candidate.h"
#include <cassert>
#include <iostream>
struct Item{unsigned code;};struct Node{Item* data;Node* next;unsigned hash;};
struct Adapter{Node node;Node* slot=nullptr;unsigned count=0,threshold=512,error=99,inserts=0,replaces=0,resizes=0;bool fail=false;
 unsigned itemCount(){return count;}void setItemCount(unsigned n){count=n;}unsigned activeBuckets(){return 8;}unsigned growThreshold(){return threshold;}void resizeContainer(){assert(!error);++resizes;}
 void setLookupError(unsigned n){error=n;}unsigned lookupError(){return error;}Node** searchItemSlot(Item*,unsigned* hash){assert(!error);*hash=7;return &slot;}Node* allocateNode(){return fail?nullptr:&node;}
 unsigned insertions(){return inserts;}void setInsertions(unsigned n){inserts=n;}unsigned replacements(){return replaces;}void setReplacements(unsigned n){replaces=n;}
};
int main(){Item first{7},second{7};Adapter a;assert(!insertControllerContainerCandidate(&first,a) && a.count==1 && a.inserts==1 && a.slot==&a.node && a.node.hash==7 && !a.node.next);assert(insertControllerContainerCandidate(&second,a)==&first && a.count==1 && a.replaces==1 && a.node.data==&second && a.node.hash==7);
 Adapter b;b.fail=true;assert(!insertControllerContainerCandidate(&first,b) && b.error==1 && !b.count && !b.slot);
 Adapter c;c.count=16;insertControllerContainerCandidate(&first,c);assert(c.resizes==1 && c.count==17);
 std::cout<<"x64 insertion: publish/count, replacement returns old payload, allocation failure and threshold boundary passed\n";
}
