#include "graph_bookmark_undo_snapshot_candidate.h"
#include <vector>
#include <cassert>
#include <iostream>
struct Adapter{
 std::vector<std::uint32_t> data;std::uint32_t* buffer=nullptr;int count=0;unsigned allocations=0;
 auto* allocate1025ea80(std::uint32_t bytes){++allocations;data.resize(bytes/4);return data.data();}
 void setBuffer(std::uint32_t* p){buffer=p;if(p)for(auto value:data)assert(value==0x80000000u);}
 auto* liveBuffer(){return buffer;}void setCount(int n){count=n;}
};
int main(){
 assert(graphBookmarkUndoAllocationBytes(0x3fffffff)==0xfffffffcu);
 assert(graphBookmarkUndoAllocationBytes(0x40000000)==0xffffffffu);
 std::vector<std::uint32_t> source(4097);for(unsigned i=0;i<source.size();++i)source[i]=i*0x9e3779b9u;
 Adapter a;graphBookmarkUndoSnapshotCandidate(source.data(),4097,a);assert(a.data==source&&a.count==4097);
 source[0]^=1;assert(a.data[0]!=source[0]);
 Adapter b;graphBookmarkUndoSnapshotCandidate(nullptr,10,b);assert(!b.allocations&&!b.count&&!b.buffer);
 graphBookmarkUndoSnapshotCandidate(source.data(),-1,b);assert(!b.allocations&&!b.count);
 std::cout<<"x64 bookmark undo snapshot: owned rawtime copy, initialization, empty gates and allocation-size saturation passed\n";
}
