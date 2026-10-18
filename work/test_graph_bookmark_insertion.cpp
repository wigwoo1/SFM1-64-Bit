#include "graph_bookmark_growth_candidate.h"
#include "graph_bookmark_shift_candidate.h"
#include <vector>
#include <cassert>
#include <iostream>
struct Adapter{std::vector<unsigned> data{1,2,3,4};int count=4;unsigned* cached=nullptr;unsigned growth=0,shifts=0;std::uint32_t cap=4;
 int count0c(){return count;}int capacity4(){return int(cap);}void setCount0c(int n){count=n;}
 void grow102bdbd0(int n){++growth;graphBookmarkGrowthCandidate(std::uint32_t(n),*this);}
 std::uint32_t growSize(){return 0;}auto capacity(){return cap;}auto* sourceData(){return data.data();}void setCapacity(std::uint32_t n){cap=n;}void setStorage(unsigned* p){assert(p==data.data());}unsigned* allocate(std::uint32_t bytes){data.resize(bytes/4);return data.data();}unsigned* reallocate(unsigned*,std::uint32_t bytes){return allocate(bytes);}unsigned* base0(){return data.data();}void setCachedBase10(unsigned* p){cached=p;}
 void shift10974fe0(unsigned* base,int start,int amount,unsigned bytes){++shifts;graphBookmarkShiftCandidate(base+start+amount,base+start,bytes);}unsigned* insertedSlot(int start,int i){return data.data()+start+i;}
};
int main(){Adapter a;assert(graphBookmarkInsertionCandidate(2,3,a)==2 && a.count==7 && a.growth==1 && a.shifts==1 && a.cached==a.data.data());assert(a.cap==8 && (std::vector<unsigned>(a.data.begin(),a.data.begin()+a.count)==std::vector<unsigned>{1,2,0x80000000,0x80000000,0x80000000,3,4}));Adapter b;assert(graphBookmarkInsertionCandidate(4,0,b)==4 && !b.growth && !b.shifts && !b.cached);graphBookmarkInsertionCandidate(4,1,b);assert(b.data[4]==0x80000000 && !b.shifts);for(int size=0;size<=128;++size)for(int start=0;start<=size;++start)for(int amount=0;amount<=8;++amount){Adapter f;f.data.resize(size);f.cap=size;f.count=size;for(int i=0;i<size;++i)f.data[i]=unsigned(i)*2654435761u;auto expected=f.data;expected.insert(expected.begin()+start,amount,0x80000000u);graphBookmarkInsertionCandidate(start,amount,f);assert(f.count==size+amount);assert(std::vector<unsigned>(f.data.begin(),f.data.begin()+f.count)==expected);}
 std::cout<<"x64 bookmark insertion: middle shift, invalid-time initialization, end growth, zero-amount gate and 75465 composed insertion cases passed\n";}
