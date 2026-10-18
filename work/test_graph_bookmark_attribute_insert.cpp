#include "graph_bookmark_attribute_insert_candidate.h"
#include "graph_bookmark_single_insert_candidate.h"
#include "graph_bookmark_storage_composition_fixture.h"
#include <iostream>
struct Adapter{
 GraphBookmarkStorageFixture storage;int attribute=0,owner=0,undo=0;bool allowed=true,record=false,fail=false;
 unsigned dirty=0,savedValue=0;int savedIndex=-1;std::vector<int> events;
 auto* attribute4(){return &attribute;}auto* owner8(int*){return &owner;}auto* vector8(){return &storage;}
 unsigned mutationGate103623b0(int*){events.push_back(1);return allowed;}
 unsigned recordModelSlot62(int*){events.push_back(2);return record;}
 int* allocateNativeInsertUndo(){events.push_back(3);return fail?nullptr:&undo;}
 void constructInsertIdentity10364bd0(int*,int*){events.push_back(4);}
 void setInsertTable(int*,std::uintptr_t tag){assert(tag==0x10b386dc);}
 void setInsertIndex1c(int*,int n){savedIndex=n;}void setInsertValue20(int*,unsigned n){savedValue=n;}
 void submitModelSlot79(int* p){assert(p==(fail?nullptr:&undo));events.push_back(5);}
 int insert1008f550(GraphBookmarkStorageFixture* s,int n,const unsigned* v){events.push_back(6);return graphBookmarkSingleInsertCandidate(n,v,*s);}
 unsigned flags11(int*){return 1;}unsigned ownerReady1035dc00(int*){return 1;}
 void ownerSlot1(int*,int*,int start,int end){assert(start==end&&storage.count>start);events.push_back(7);}
 void ownerSlot0(int*,int*){events.push_back(8);}unsigned type10(int*){return 21;}
 void markOwner1035eff0(int*,bool refs){assert(!refs);events.push_back(9);}
 void orFrameworkFlags(unsigned flags){dirty|=flags;events.push_back(10);}
};
int main(){unsigned value=0x80000000;Adapter a;assert(graphBookmarkAttributeInsertCandidate(0,&value,a)==0);
 assert(a.storage.count==1&&a.storage.data[0]==value&&a.dirty==0x4000&&(a.events==std::vector<int>{1,2,6,7,8,9,10}));
 Adapter b;b.record=true;graphBookmarkAttributeInsertCandidate(0,&value,b);assert(b.savedIndex==0&&b.savedValue==value&&(b.events==std::vector<int>{1,2,3,4,5,6,7,8,9,10}));
 Adapter c;c.record=true;c.fail=true;graphBookmarkAttributeInsertCandidate(0,&value,c);assert((c.events==std::vector<int>{1,2,3,5,6,7,8,9,10}));
 Adapter d;d.allowed=false;assert(graphBookmarkAttributeInsertCandidate(0,&value,d)==-1&&!d.storage.count&&(d.events==std::vector<int>{1}));
 std::cout<<"x64 attribute bookmark insertion: composed storage, undo capture/null submission, notification order and mutation gate passed\n";
}
