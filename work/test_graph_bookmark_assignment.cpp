#include "graph_bookmark_assignment_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Adapter{unsigned data[4]={};int size=2,owner=1,undo=2,type=8;bool allowed=true,record=false,fail=false;unsigned flags=1,dirty=0;std::vector<int> calls;
 unsigned* destinationBase(){return data;}int destinationCount(){return size;}void remove103883e0(int,int){calls.push_back(20);}void insert103771c0(int,int){calls.push_back(21);}
 unsigned mutationGate103623b0(){calls.push_back(1);return allowed;}unsigned recordOwnerModelSlot62(){calls.push_back(2);return record;}
 int* allocateNativeUndo(){calls.push_back(3);return fail?nullptr:&undo;}int* construct1036c020(int* p,unsigned*,int n,int z){assert(n==3 && !z);calls.push_back(4);return p;}
 void submitModelSlot79(int* p){assert(p==(fail?nullptr:&undo));calls.push_back(5);}int* owner8(){return &owner;}unsigned flags11(){return flags;}unsigned ownerReady1035dc00(int*){return 1;}
 void ownerSlot2(int*,int z,int end){assert(!z && end==1);calls.push_back(6);}void copy1038a010(unsigned*,int n){size=n;calls.push_back(7);}
 void ownerSlot1(int*,int z,int end){assert(!z && end==2);calls.push_back(8);}void ownerSlot0(int*){calls.push_back(9);}
 unsigned type10(){return type;}void mark1035eff0(bool reference){assert(reference==(type==15));calls.push_back(10);}void orFrameworkFlags(unsigned f){dirty|=f;calls.push_back(11);}
};
int main(){unsigned source[3]={1,2,3};Adapter a;graphBookmarkAssignmentCandidate(source,3,a);assert((a.calls==std::vector<int>{1,2,6,7,8,9,10,11}) && a.dirty==0x4000);Adapter b;b.record=true;b.fail=true;b.type=15;graphBookmarkAssignmentCandidate(source,3,b);assert((b.calls==std::vector<int>{1,2,3,5,6,7,8,9,10,11}) && b.dirty==0x1000);Adapter c;c.allowed=false;graphBookmarkAssignmentCandidate(source,3,c);assert((c.calls==std::vector<int>{1}));Adapter d;graphBookmarkAssignmentCandidate(d.data,3,d);assert((d.calls==std::vector<int>{21}));std::cout<<"x64 bookmark assignment: mutation/alias gates, failed undo submission, notification order and dirty flags passed\n";}
