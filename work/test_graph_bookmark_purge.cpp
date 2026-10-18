#include "graph_bookmark_purge_candidate.h"
#include <vector>
#include "graph_bookmark_storage_purge_candidate.h"
#include <cassert>
#include <iostream>
struct Attribute{int owner=1;unsigned flags=1,type=21;};struct Vector{int count=3,grow=0,capacity=3;unsigned* base=nullptr;unsigned* cached=nullptr;unsigned frees=0;
 void setCount0c(int n){count=n;}int growSize8(){return grow;}auto* base0(){return base;}
 void freeAllocatorSlot5(unsigned* p){assert(!count);++frees;delete[] p;}
 void setBase0(unsigned* p){base=p;}void setCapacity4(int n){capacity=n;}void setCachedBase10(unsigned* p){cached=p;}
};
struct Adapter{
 Attribute attribute;Vector original,replacement{7};Vector* live=&original;
 bool allowed=true,record=false,fail=false,mutate=false;int undo=0;unsigned dirty=0;std::vector<int> calls;
 auto* attribute4(){return &attribute;}auto* vector8(){return live;}
 unsigned mutationGate103623b0(Attribute*){calls.push_back(1);return allowed;}
 auto* owner8(Attribute* p){return &p->owner;}
 unsigned recordModelSlot62(int*){calls.push_back(2);return record;}
 int* allocateNativeUndo(){calls.push_back(3);return fail?nullptr:&undo;}
 int* construct1036c020(int* p,Attribute*,std::nullptr_t,int n,int flag){assert(n==1&&!flag);calls.push_back(4);return p;}
 void submitModelSlot79(int* p){assert(p==(fail?nullptr:&undo));calls.push_back(5);}
 unsigned flags11(Attribute* p){return p->flags;}int count0c(Vector* p){return p->count;}
 unsigned ownerReady1035dc00(int*){return 1;}
 void ownerSlot2(int*,Attribute*,int start,int end){assert(!start&&end==original.count-1);calls.push_back(6);if(mutate)live=&replacement;}
 void purgeStorage(Vector* p){calls.push_back(7);graphBookmarkStoragePurgeCandidate(*p);}
 void ownerSlot0(int*,Attribute*){calls.push_back(8);}
 unsigned type10(Attribute* p){return p->type;}
 void markOwner1035eff0(int*,bool references){assert(references==(attribute.type==15));calls.push_back(9);}
 void orFrameworkFlags(unsigned bits){dirty|=bits;calls.push_back(10);}
};
int main(){
 Adapter a;a.original.base=new unsigned[3];graphBookmarkPurgeCandidate(a);assert(a.original.frees==1&&!a.original.base&&!a.original.capacity);assert(!a.original.count&&a.dirty==0x4000&&(a.calls==std::vector<int>{1,2,6,7,8,9,10}));
 Adapter b;b.mutate=true;graphBookmarkPurgeCandidate(b);assert(b.original.count==3&&!b.replacement.count);
 Adapter c;c.record=true;c.fail=true;c.attribute.type=15;graphBookmarkPurgeCandidate(c);assert(c.dirty==0x1000&&(c.calls==std::vector<int>{1,2,3,5,6,7,8,9,10}));
 Adapter d;d.allowed=false;graphBookmarkPurgeCandidate(d);assert(d.original.count==3&&(d.calls==std::vector<int>{1}));
 Adapter e;e.original.count=0;graphBookmarkPurgeCandidate(e);assert(!e.original.count);
 Adapter f;f.record=true;graphBookmarkPurgeCandidate(f);assert((f.calls==std::vector<int>{1,2,3,4,5,6,7,8,9,10}));
 unsigned external[3]{};Adapter g;g.original.grow=-1;g.original.base=external;graphBookmarkPurgeCandidate(g);assert(!g.original.count&&g.original.capacity==3&&g.original.base==external&&g.original.cached==external&&!g.original.frees);
 std::cout<<"x64 attribute bookmark purge: owned/external storage, undo arguments, notification order, live vector, null undo and mutation gate passed\n";
}
