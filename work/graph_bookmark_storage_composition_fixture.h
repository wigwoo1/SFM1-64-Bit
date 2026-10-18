#pragma once
#include "graph_bookmark_growth_candidate.h"
#include "graph_bookmark_shift_candidate.h"
#include "graph_bookmark_copy_candidate.h"
#include <vector>
#include <cassert>
struct GraphBookmarkStorageFixture {
 std::vector<std::uint32_t> data;int count=0;std::uint32_t cap=0;
 std::uint32_t* cached=nullptr;unsigned growth=0,shifts=0;
 int count0c(){return count;}int capacity4(){return graphInsertSigned(cap);}
 void setCount0c(int n){count=n;}auto* base0(){return data.data();}
 void setCachedBase10(std::uint32_t* p){cached=p;}
 void grow102bdbd0(int n){++growth;graphBookmarkGrowthCandidate(std::uint32_t(n),*this);}
 std::uint32_t growSize(){return 0;}auto capacity(){return cap;}
 auto* sourceData(){return data.data();}void setCapacity(std::uint32_t n){cap=n;}
 void setStorage(std::uint32_t* p){assert(p==data.data());}
 std::uint32_t* allocate(std::uint32_t bytes){data.resize(bytes/4);return data.data();}
 std::uint32_t* reallocate(std::uint32_t*,std::uint32_t bytes){return allocate(bytes);}
 void shift10974fe0(std::uint32_t* base,int start,int amount,std::uint32_t bytes){
  ++shifts;graphBookmarkShiftCandidate(base+start+amount,base+start,bytes);
 }
 auto* insertedSlot(int start,int i){return data.data()+start+i;}
};
struct GraphBookmarkCopyFixture {
 GraphBookmarkStorageFixture storage;unsigned inserts=0,reads=0;
 auto* vector8(){return &storage;}
 void setCount0c(GraphBookmarkStorageFixture* s,int n){s->count=n;}
 void insert1017ee80(GraphBookmarkStorageFixture* s,int start,int count){
  assert(s->count==0&&start==0);++inserts;graphBookmarkInsertionCandidate(start,count,*s);
 }
 auto* liveBase0(GraphBookmarkStorageFixture* s){++reads;return s->base0();}
};
