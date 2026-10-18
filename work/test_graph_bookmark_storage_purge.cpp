#include "graph_bookmark_storage_purge_candidate.h"
#include <cassert>
#include <iostream>
struct Adapter{
 unsigned external[2]{},*base=nullptr,*cached=nullptr;int grow=0,count=2,capacity=2;unsigned frees=0;
 void setCount0c(int n){count=n;}int growSize8(){return grow;}auto* base0(){return base;}
 void freeAllocatorSlot5(unsigned* p){assert(!count);++frees;delete[] p;base=external;}
 void setBase0(unsigned* p){base=p;}void setCapacity4(int n){capacity=n;}
 void setCachedBase10(unsigned* p){cached=p;}
};
int main(){Adapter a;a.base=new unsigned[2];graphBookmarkStoragePurgeCandidate(a);assert(a.frees==1&&!a.count&&!a.capacity&&!a.base&&!a.cached);
 Adapter b;b.grow=-1;b.base=b.external;graphBookmarkStoragePurgeCandidate(b);assert(!b.frees&&!b.count&&b.capacity==2&&b.base==b.external&&b.cached==b.external);
 Adapter c;graphBookmarkStoragePurgeCandidate(c);assert(!c.frees&&!c.capacity&&!c.cached);
 std::cout<<"x64 bookmark purge: owned release, callback pointer reset, external retention and null storage passed\n";
}
