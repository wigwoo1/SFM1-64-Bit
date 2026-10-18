#include "graph_bookmark_clear_candidate.h"
#include <vector>
#include <cassert>
#include <iostream>
struct Attribute{int owner=1;unsigned flags=1,type=21;};struct Vector{int count=3;};
struct Adapter{
 Attribute attribute;Vector original,replacement{7};Vector* live=&original;
 bool allowed=true,record=false,fail=false,mutate=false;int undo=0;unsigned dirty=0;std::vector<int> calls;
 auto* attribute4(){return &attribute;}auto* vector8(){return live;}
 unsigned mutationGate103623b0(Attribute*){calls.push_back(1);return allowed;}
 auto* owner8(Attribute* p){return &p->owner;}
 unsigned recordModelSlot62(int*){calls.push_back(2);return record;}
 int* allocateNativeUndo(){calls.push_back(3);return fail?nullptr:&undo;}
 int* construct1036c020(int* p,Attribute*,std::nullptr_t,int n,int flag){assert(!n&&!flag);calls.push_back(4);return p;}
 void submitModelSlot79(int* p){assert(p==(fail?nullptr:&undo));calls.push_back(5);}
 unsigned flags11(Attribute* p){return p->flags;}int count0c(Vector* p){return p->count;}
 unsigned ownerReady1035dc00(int*){return 1;}
 void ownerSlot2(int*,Attribute*,int start,int end){assert(!start&&end==original.count-1);calls.push_back(6);if(mutate)live=&replacement;}
 void setCount0c(Vector* p,int n){calls.push_back(7);p->count=n;}
 void ownerSlot0(int*,Attribute*){calls.push_back(8);}
 unsigned type10(Attribute* p){return p->type;}
 void markOwner1035eff0(int*,bool references){assert(references==(attribute.type==15));calls.push_back(9);}
 void orFrameworkFlags(unsigned bits){dirty|=bits;calls.push_back(10);}
};
int main(){
 Adapter a;graphBookmarkClearCandidate(a);assert(!a.original.count&&a.dirty==0x4000&&(a.calls==std::vector<int>{1,2,6,7,8,9,10}));
 Adapter b;b.mutate=true;graphBookmarkClearCandidate(b);assert(b.original.count==3&&!b.replacement.count);
 Adapter c;c.record=true;c.fail=true;c.attribute.type=15;graphBookmarkClearCandidate(c);assert(c.dirty==0x1000&&(c.calls==std::vector<int>{1,2,3,5,6,7,8,9,10}));
 Adapter d;d.allowed=false;graphBookmarkClearCandidate(d);assert(d.original.count==3&&(d.calls==std::vector<int>{1}));
 Adapter e;e.original.count=0;graphBookmarkClearCandidate(e);assert(!e.original.count);
 std::cout<<"x64 bookmark clear: notification order, live storage, failed undo submission, empty range and mutation gate passed\n";
}
