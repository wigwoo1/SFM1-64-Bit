#include "graph_bookmark_growth_candidate.h"
#include <cassert>
#include <iostream>
#include <vector>
struct Adapter {
 std::uint32_t grow=0,cap=0,bytes=0;std::vector<unsigned> data;unsigned* ptr=nullptr;bool fail=false;unsigned calls=0;
 auto growSize(){return grow;}auto capacity(){return cap;}auto* sourceData(){return ptr;}
 void setCapacity(std::uint32_t n){cap=n;}void setStorage(unsigned* p){ptr=p;}
 unsigned* allocate(std::uint32_t n){++calls;bytes=n;assert(n==cap*4u);if(fail)return nullptr;data.resize(n/4);return data.data();}
 unsigned* reallocate(unsigned* p,std::uint32_t n){assert(p==data.data());return allocate(n);}
};
int main(){
 Adapter a;graphBookmarkGrowthCandidate(1,a);assert(a.cap==8&&a.bytes==32);a.data[0]=123;
 graphBookmarkGrowthCandidate(9,a);assert(a.cap==32&&a.data[0]==123&&a.bytes==128);
 Adapter b;b.grow=6;graphBookmarkGrowthCandidate(7,b);assert(b.cap==12&&b.bytes==48);
 b.fail=true;graphBookmarkGrowthCandidate(1,b);assert(b.cap==18&&!b.ptr&&b.bytes==72);
 Adapter c;c.grow=0xffffffff;graphBookmarkGrowthCandidate(99,c);assert(!c.calls&&!c.cap);
 std::cout<<"x64 bookmark growth: geometric/explicit capacity, preserved data, external-storage gate and allocation failure passed\n";
}
